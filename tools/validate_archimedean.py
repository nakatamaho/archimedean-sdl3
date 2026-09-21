#!/usr/bin/env python3
"""Validate the canonical regular-solid JSON with ordinary CPython."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any, Sequence

from expected_archimedean import EXPECTED_SOLIDS, SOLID_ORDER


NORMALIZED_EDGE_TOLERANCE = 1.0e-8
PLANARITY_TOLERANCE_FACTOR = 1.0e-9
CENTROID_TOLERANCE_FACTOR = 1.0e-12
ZERO_TOLERANCE = 1.0e-12

Vector = tuple[float, float, float]


class ValidationError(ValueError):
    """Raised when a JSON document violates the canonical data contract."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def _is_integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _vector(value: Any, context: str) -> Vector:
    _require(isinstance(value, list) and len(value) == 3, f"{context}: expected Vec3")
    _require(
        all(isinstance(component, (int, float)) and not isinstance(component, bool)
            for component in value),
        f"{context}: coordinates must be numbers",
    )
    result = tuple(float(component) for component in value)
    _require(all(math.isfinite(component) for component in result),
             f"{context}: coordinate is not finite")
    return result


def _subtract(left: Vector, right: Vector) -> Vector:
    return (
        left[0] - right[0],
        left[1] - right[1],
        left[2] - right[2],
    )


def _add(left: Vector, right: Vector) -> Vector:
    return (
        left[0] + right[0],
        left[1] + right[1],
        left[2] + right[2],
    )


def _scale(vector: Vector, factor: float) -> Vector:
    return (vector[0] * factor, vector[1] * factor, vector[2] * factor)


def _dot(left: Vector, right: Vector) -> float:
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2]


def _cross(left: Vector, right: Vector) -> Vector:
    return (
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0],
    )


def _length(vector: Vector) -> float:
    return math.sqrt(_dot(vector, vector))


def _unit(vector: Vector, context: str) -> Vector:
    length = _length(vector)
    _require(length > ZERO_TOLERANCE, f"{context}: zero-length normal")
    return _scale(vector, 1.0 / length)


def _face_normal(face: Sequence[int], vertices: Sequence[Vector]) -> Vector:
    normal = (0.0, 0.0, 0.0)
    for index, vertex_index in enumerate(face):
        next_vertex = vertices[face[(index + 1) % len(face)]]
        normal = _add(normal, _cross(vertices[vertex_index], next_vertex))
    return normal


def _face_center(face: Sequence[int], vertices: Sequence[Vector]) -> Vector:
    return _scale(
        (
            sum(vertices[index][0] for index in face),
            sum(vertices[index][1] for index in face),
            sum(vertices[index][2] for index in face),
        ),
        1.0 / len(face),
    )


def _parse_constant(value: str) -> None:
    raise ValidationError(f"non-standard JSON constant {value}")


def load_document(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as stream:
            document = json.load(stream, parse_constant=_parse_constant)
    except json.JSONDecodeError as error:
        raise ValidationError(f"malformed JSON: {error}") from error
    _require(isinstance(document, dict), "top level must be an object")
    return document


def validate_solid(solid: Any, position: int) -> None:
    context = f"solid[{position}]"
    _require(isinstance(solid, dict), f"{context}: expected an object")
    solid_id = solid.get("id")
    _require(isinstance(solid_id, str), f"{context}: missing string id")
    _require(solid_id in EXPECTED_SOLIDS, f"{context}: unsupported id {solid_id}")
    expected = EXPECTED_SOLIDS[solid_id]
    _require(
        solid.get("display_name") == expected["display_name"],
        f"{solid_id}: display name mismatch",
    )

    raw_vertices = solid.get("vertices")
    _require(isinstance(raw_vertices, list), f"{solid_id}: vertices must be an array")
    vertices = [_vector(value, f"{solid_id}.vertices[{index}]")
                for index, value in enumerate(raw_vertices)]
    _require(len(vertices) == expected["vertices"],
             f"{solid_id}: unexpected vertex count {len(vertices)}")
    _require(len(set(vertices)) == len(vertices),
             f"{solid_id}: duplicate vertices")

    raw_faces = solid.get("faces")
    _require(isinstance(raw_faces, list), f"{solid_id}: faces must be an array")
    _require(len(raw_faces) == expected["faces"],
             f"{solid_id}: unexpected face count {len(raw_faces)}")

    actual_edges: set[tuple[int, int]] = set()
    incidence: dict[tuple[int, int], int] = {}
    histogram: dict[str, int] = {}
    radius = max(_length(vertex) for vertex in vertices)
    _require(radius > ZERO_TOLERANCE, f"{solid_id}: zero characteristic radius")
    centroid = _scale(
        (
            sum(vertex[0] for vertex in vertices),
            sum(vertex[1] for vertex in vertices),
            sum(vertex[2] for vertex in vertices),
        ),
        1.0 / len(vertices),
    )
    _require(_length(centroid) <= CENTROID_TOLERANCE_FACTOR * radius,
             f"{solid_id}: vertex centroid is not at the origin")

    for face_position, raw_face in enumerate(raw_faces):
        face_context = f"{solid_id}.faces[{face_position}]"
        _require(isinstance(raw_face, list) and len(raw_face) >= 3,
                 f"{face_context}: expected a polygon")
        _require(all(_is_integer(index) for index in raw_face),
                 f"{face_context}: indices must be integers")
        face = list(raw_face)
        _require(all(0 <= index < len(vertices) for index in face),
                 f"{face_context}: vertex index out of range")
        _require(len(set(face)) == len(face),
                 f"{face_context}: duplicate vertex")
        face_edges = [
            tuple(sorted((face[index], face[(index + 1) % len(face)])))
            for index in range(len(face))
        ]
        _require(len(set(face_edges)) == len(face_edges),
                 f"{face_context}: duplicate edge")
        for edge in face_edges:
            actual_edges.add(edge)
            incidence[edge] = incidence.get(edge, 0) + 1

        normal = _unit(_face_normal(face, vertices), face_context)
        reference = vertices[face[0]]
        planarity = max(
            abs(_dot(normal, _subtract(vertices[index], reference)))
            for index in face
        )
        _require(planarity <= PLANARITY_TOLERANCE_FACTOR * radius,
                 f"{face_context}: non-planar face")
        _require(_dot(normal, _face_center(face, vertices)) > ZERO_TOLERANCE,
                 f"{face_context}: face winding is not outward")
        key = str(len(face))
        histogram[key] = histogram.get(key, 0) + 1

    _require(all(count == 2 for count in incidence.values()),
             f"{solid_id}: edge incidence is not two")
    _require(len(actual_edges) == expected["edges"],
             f"{solid_id}: unexpected edge count {len(actual_edges)}")
    _require(len(vertices) - len(actual_edges) + len(raw_faces) == 2,
             f"{solid_id}: Euler characteristic is not two")
    _require(histogram == expected["face_histogram"],
             f"{solid_id}: face histogram mismatch")

    raw_edges = solid.get("edges")
    _require(isinstance(raw_edges, list), f"{solid_id}: edges must be an array")
    parsed_edges: list[tuple[int, int]] = []
    for edge_position, raw_edge in enumerate(raw_edges):
        context_edge = f"{solid_id}.edges[{edge_position}]"
        _require(isinstance(raw_edge, list) and len(raw_edge) == 2,
                 f"{context_edge}: expected a pair")
        _require(all(_is_integer(index) for index in raw_edge),
                 f"{context_edge}: indices must be integers")
        left, right = raw_edge
        _require(0 <= left < len(vertices) and 0 <= right < len(vertices),
                 f"{context_edge}: vertex index out of range")
        _require(left < right, f"{context_edge}: edge is not canonical")
        parsed_edges.append((left, right))
    _require(parsed_edges == sorted(actual_edges),
             f"{solid_id}: explicit edges do not match polygon topology")

    lengths = [
        _length(_subtract(vertices[left], vertices[right]))
        for left, right in sorted(actual_edges)
    ]
    mean_edge = sum(lengths) / len(lengths)
    spread = (max(lengths) - min(lengths)) / mean_edge
    _require(abs(mean_edge - 1.0) <= NORMALIZED_EDGE_TOLERANCE,
             f"{solid_id}: mean edge length is not one")
    _require(spread <= NORMALIZED_EDGE_TOLERANCE,
             f"{solid_id}: normalized edge spread is {spread}")

    statistics = solid.get("statistics")
    _require(
        statistics == {
            "vertices": len(vertices),
            "edges": len(actual_edges),
            "faces": len(raw_faces),
            "face_histogram": dict(
                sorted(histogram.items(), key=lambda item: int(item[0]))
            ),
        },
        f"{solid_id}: statistics mismatch",
    )


def validate_document(document: dict[str, Any]) -> None:
    _require(document.get("schema_version") == 1, "unsupported schema version")
    generator = document.get("generator")
    _require(isinstance(generator, dict), "generator metadata is missing")
    _require(generator.get("program") == "tools/generate_archimedean.py",
             "generator program metadata mismatch")
    _require(isinstance(generator.get("sage_version"), str)
             and generator["sage_version"], "Sage version metadata is missing")
    solids = document.get("solids")
    _require(isinstance(solids, list), "solids must be an array")
    _require([solid.get("id") if isinstance(solid, dict) else None for solid in solids]
             == list(SOLID_ORDER), "solid order or membership mismatch")
    for position, solid in enumerate(solids):
        validate_solid(solid, position)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path, help="JSON document to validate")
    arguments = parser.parse_args()
    try:
        document = load_document(arguments.path)
        validate_document(document)
    except (OSError, ValidationError) as error:
        print(f"FAIL: {error}")
        return 1
    print(f"PASS: validated {len(document['solids'])} solids")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
