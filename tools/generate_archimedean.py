#!/usr/bin/env python3
"""Generate deterministic polygon data from SageMath polytope constructors."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Callable, Iterable, Sequence

from sage.all import AA, polytopes
from sage.env import SAGE_VERSION

from expected_archimedean import EXPECTED_SOLIDS, SOLID_ORDER


NORMALIZED_EDGE_TOLERANCE = 1.0e-8
PLANARITY_TOLERANCE_FACTOR = 1.0e-9
ZERO_TOLERANCE = 1.0e-12

Vector = tuple[float, float, float]
Face = list[int]


def _add(left: Vector, right: Vector) -> Vector:
    return (
        left[0] + right[0],
        left[1] + right[1],
        left[2] + right[2],
    )


def _subtract(left: Vector, right: Vector) -> Vector:
    return (
        left[0] - right[0],
        left[1] - right[1],
        left[2] - right[2],
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


def _unit(vector: Vector) -> Vector:
    length = _length(vector)
    if length <= ZERO_TOLERANCE:
        raise ValueError("encountered a zero-length vector")
    return _scale(vector, 1.0 / length)


def _clean_float(value: float) -> float:
    if not math.isfinite(value):
        raise ValueError("non-finite coordinate generated")
    if value == 0.0:
        return 0.0
    return value


def _vertex_coordinates(polyhedron) -> list[Vector]:
    coordinates: list[Vector] = []
    for vertex in polyhedron.vertices():
        values = tuple(float(component) for component in vertex.vector())
        if len(values) != 3:
            raise ValueError("expected a three-dimensional polyhedron")
        coordinates.append(values)
    if len({coordinate for coordinate in coordinates}) != len(coordinates):
        raise ValueError("polyhedron contains duplicate vertices")
    return coordinates


def _raw_faces(polyhedron, coordinates: Sequence[Vector]) -> list[tuple[int, ...]]:
    faces: list[tuple[int, ...]] = []
    for face in polyhedron.faces(2):
        vertices = [vertex.index() for vertex in face.vertices()]
        reference = coordinates[vertices[0]]
        first_edge = _subtract(coordinates[vertices[1]], reference)
        normal = None
        for vertex_index in vertices[2:]:
            candidate = _cross(
                first_edge, _subtract(coordinates[vertex_index], reference)
            )
            if _length(candidate) > ZERO_TOLERANCE:
                normal = _unit(candidate)
                break
        if normal is None:
            raise ValueError("face has no non-collinear vertices")
        basis_u = _unit(first_edge)
        basis_v = _unit(_cross(normal, basis_u))
        center = _scale(
            (
                sum(coordinates[index][0] for index in vertices),
                sum(coordinates[index][1] for index in vertices),
                sum(coordinates[index][2] for index in vertices),
            ),
            1.0 / len(vertices),
        )
        vertices.sort(
            key=lambda index: math.atan2(
                _dot(_subtract(coordinates[index], center), basis_v),
                _dot(_subtract(coordinates[index], center), basis_u),
            )
        )
        faces.append(tuple(vertices))
    return faces


def _normalize(
    coordinates: Sequence[Vector], faces: Sequence[Sequence[int]]
) -> list[Vector]:
    centroid = _scale(
        (
            sum(vertex[0] for vertex in coordinates),
            sum(vertex[1] for vertex in coordinates),
            sum(vertex[2] for vertex in coordinates),
        ),
        1.0 / len(coordinates),
    )
    centered = [_subtract(vertex, centroid) for vertex in coordinates]

    edges = {
        tuple(sorted((face[index], face[(index + 1) % len(face)])))
        for face in faces
        for index in range(len(face))
    }
    lengths = [
        _length(_subtract(centered[left], centered[right]))
        for left, right in sorted(edges)
    ]
    mean_edge = sum(lengths) / len(lengths)
    if mean_edge <= ZERO_TOLERANCE:
        raise ValueError("cannot normalize a degenerate polyhedron")
    return [
        tuple(_clean_float(component) for component in _scale(vertex, 1.0 / mean_edge))
        for vertex in centered
    ]


def _face_normal(face: Sequence[int], coordinates: Sequence[Vector]) -> Vector:
    normal = (0.0, 0.0, 0.0)
    for index, vertex_index in enumerate(face):
        next_vertex = coordinates[face[(index + 1) % len(face)]]
        normal = _add(normal, _cross(coordinates[vertex_index], next_vertex))
    return normal


def _canonical_faces(
    raw_faces: Sequence[Sequence[int]],
    coordinates: Sequence[Vector],
) -> tuple[list[Vector], list[Face]]:
    order = sorted(
        range(len(coordinates)),
        key=lambda index: tuple(_clean_float(value) for value in coordinates[index]),
    )
    old_to_new = {old: new for new, old in enumerate(order)}
    sorted_coordinates = [coordinates[index] for index in order]

    canonical: list[Face] = []
    for raw_face in raw_faces:
        face = [old_to_new[index] for index in raw_face]
        normal = _face_normal(face, sorted_coordinates)
        center = _scale(
            (
                sum(sorted_coordinates[index][0] for index in face),
                sum(sorted_coordinates[index][1] for index in face),
                sum(sorted_coordinates[index][2] for index in face),
            ),
            1.0 / len(face),
        )
        if _dot(normal, center) < 0.0:
            face.reverse()
        if _dot(_face_normal(face, sorted_coordinates), center) <= ZERO_TOLERANCE:
            raise ValueError("face winding is not outward")
        start = min(range(len(face)), key=lambda index: face[index])
        face = face[start:] + face[:start]
        canonical.append(face)

    canonical.sort(key=lambda face: (len(face), tuple(face)))
    return sorted_coordinates, canonical


def _validate_solid(
    solid_id: str,
    coordinates: Sequence[Vector],
    faces: Sequence[Sequence[int]],
) -> dict[str, object]:
    expected = EXPECTED_SOLIDS[solid_id]
    if len(coordinates) != expected["vertices"]:
        raise ValueError(f"{solid_id}: unexpected vertex count")
    if len(faces) != expected["faces"]:
        raise ValueError(f"{solid_id}: unexpected face count")

    histogram: dict[str, int] = {}
    edge_incidence: dict[tuple[int, int], int] = {}
    characteristic_radius = max(_length(vertex) for vertex in coordinates)
    if characteristic_radius <= ZERO_TOLERANCE:
        raise ValueError(f"{solid_id}: zero characteristic radius")

    for face in faces:
        if len(face) < 3 or len(set(face)) != len(face):
            raise ValueError(f"{solid_id}: degenerate face")
        if any(index < 0 or index >= len(coordinates) for index in face):
            raise ValueError(f"{solid_id}: face index out of range")
        edges = [
            tuple(sorted((face[index], face[(index + 1) % len(face)])))
            for index in range(len(face))
        ]
        if len(set(edges)) != len(edges):
            raise ValueError(f"{solid_id}: duplicate edge inside face")
        for edge in edges:
            edge_incidence[edge] = edge_incidence.get(edge, 0) + 1

        normal = _unit(_face_normal(face, coordinates))
        reference = coordinates[face[0]]
        planarity = max(
            abs(_dot(normal, _subtract(coordinates[index], reference)))
            for index in face
        )
        if planarity > PLANARITY_TOLERANCE_FACTOR * characteristic_radius:
            raise ValueError(f"{solid_id}: face planarity tolerance exceeded")

        center = _scale(
            (
                sum(coordinates[index][0] for index in face),
                sum(coordinates[index][1] for index in face),
                sum(coordinates[index][2] for index in face),
            ),
            1.0 / len(face),
        )
        if _dot(normal, center) <= ZERO_TOLERANCE:
            raise ValueError(f"{solid_id}: face is not outward-oriented")
        key = str(len(face))
        histogram[key] = histogram.get(key, 0) + 1

    if any(count != 2 for count in edge_incidence.values()):
        raise ValueError(f"{solid_id}: every edge must have incidence two")
    if len(edge_incidence) != expected["edges"]:
        raise ValueError(f"{solid_id}: unexpected edge count")
    if len(coordinates) - len(edge_incidence) + len(faces) != 2:
        raise ValueError(f"{solid_id}: Euler characteristic is not two")
    if histogram != expected["face_histogram"]:
        raise ValueError(f"{solid_id}: unexpected face histogram {histogram}")

    lengths = [
        _length(_subtract(coordinates[left], coordinates[right]))
        for left, right in sorted(edge_incidence)
    ]
    mean_edge = sum(lengths) / len(lengths)
    spread = (max(lengths) - min(lengths)) / mean_edge
    if abs(mean_edge - 1.0) > NORMALIZED_EDGE_TOLERANCE:
        raise ValueError(f"{solid_id}: mean edge length is not one")
    if spread > NORMALIZED_EDGE_TOLERANCE:
        raise ValueError(f"{solid_id}: normalized edge spread is {spread}")

    return {
        "vertices": len(coordinates),
        "edges": len(edge_incidence),
        "faces": len(faces),
        "face_histogram": dict(sorted(histogram.items(), key=lambda item: int(item[0]))),
    }


def _constructors() -> dict[str, Callable[[], object]]:
    return {
        "tetrahedron": lambda: polytopes.tetrahedron(),
        "cube": lambda: polytopes.cube(),
        "octahedron": lambda: polytopes.octahedron(),
        "dodecahedron": lambda: polytopes.dodecahedron(exact=True),
        "icosahedron": lambda: polytopes.icosahedron(exact=True),
        "truncated_tetrahedron": lambda: polytopes.truncated_tetrahedron(),
        "cuboctahedron": lambda: polytopes.cuboctahedron(),
        "truncated_cube": lambda: polytopes.truncated_cube(exact=True),
        "truncated_octahedron": lambda: polytopes.truncated_octahedron(),
        "rhombicuboctahedron": lambda: polytopes.small_rhombicuboctahedron(exact=True),
        "truncated_cuboctahedron": lambda: polytopes.great_rhombicuboctahedron(
            exact=True
        ),
        "snub_cube": lambda: polytopes.snub_cube(exact=True),
        "icosidodecahedron": lambda: polytopes.icosidodecahedron(exact=True),
        "truncated_dodecahedron": lambda: polytopes.truncated_dodecahedron(exact=True),
        "truncated_icosahedron": lambda: polytopes.icosahedron(
            exact=True
        ).truncation(),
        "rhombicosidodecahedron": lambda: polytopes.rhombicosidodecahedron(
            exact=True
        ),
        "truncated_icosidodecahedron": lambda: polytopes.truncated_icosidodecahedron(
            exact=True
        ),
        "snub_dodecahedron": lambda: polytopes.snub_dodecahedron(
            base_ring=AA, backend="normaliz"
        ),
    }


def _generate_solid(solid_id: str, constructor: Callable[[], object]) -> dict[str, object]:
    polyhedron = constructor()
    raw_coordinates = _vertex_coordinates(polyhedron)
    raw_faces = _raw_faces(polyhedron, raw_coordinates)
    normalized = _normalize(raw_coordinates, raw_faces)
    coordinates, faces = _canonical_faces(raw_faces, normalized)
    statistics = _validate_solid(solid_id, coordinates, faces)
    return {
        "id": solid_id,
        "display_name": EXPECTED_SOLIDS[solid_id]["display_name"],
        "vertices": [[value for value in vertex] for vertex in coordinates],
        "faces": [list(face) for face in faces],
        "edges": [
            list(edge)
            for edge in sorted(
                {
                    tuple(sorted((face[index], face[(index + 1) % len(face)])))
                    for face in faces
                    for index in range(len(face))
                }
            )
        ],
        "statistics": statistics,
    }


def generate(output: Path) -> None:
    constructors = _constructors()
    if tuple(constructors) != SOLID_ORDER:
        raise ValueError("constructor order does not match the canonical solid order")
    solids = [_generate_solid(solid_id, constructors[solid_id]) for solid_id in SOLID_ORDER]
    document = {
        "schema_version": 1,
        "generator": {
            "program": "tools/generate_archimedean.py",
            "sage_version": str(SAGE_VERSION),
            "normaliz_backend": "PyNormaliz 2.23 via Sage backend='normaliz'",
            "tolerances": {
                "normalized_edge_relative": NORMALIZED_EDGE_TOLERANCE,
                "planarity_relative_radius": PLANARITY_TOLERANCE_FACTOR,
                "zero_vector": ZERO_TOLERANCE,
            },
        },
        "solids": solids,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(document, stream, indent=2, ensure_ascii=False, allow_nan=False)
        stream.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/archimedean.json"),
        help="output JSON path (default: data/archimedean.json)",
    )
    arguments = parser.parse_args()
    generate(arguments.output)
    print(f"generated {len(SOLID_ORDER)} solids at {arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
