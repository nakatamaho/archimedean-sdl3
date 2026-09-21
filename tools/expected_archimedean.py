"""Expected combinatorics for the v1 Archimedean-solid data set."""

EXPECTED_SOLIDS = {
    "truncated_tetrahedron": {
        "display_name": "Truncated tetrahedron",
        "vertices": 12,
        "edges": 18,
        "faces": 8,
        "face_histogram": {"3": 4, "6": 4},
    },
    "cuboctahedron": {
        "display_name": "Cuboctahedron",
        "vertices": 12,
        "edges": 24,
        "faces": 14,
        "face_histogram": {"3": 8, "4": 6},
    },
    "truncated_cube": {
        "display_name": "Truncated cube",
        "vertices": 24,
        "edges": 36,
        "faces": 14,
        "face_histogram": {"3": 8, "8": 6},
    },
    "truncated_octahedron": {
        "display_name": "Truncated octahedron",
        "vertices": 24,
        "edges": 36,
        "faces": 14,
        "face_histogram": {"4": 6, "6": 8},
    },
    "rhombicuboctahedron": {
        "display_name": "Rhombicuboctahedron",
        "vertices": 24,
        "edges": 48,
        "faces": 26,
        "face_histogram": {"3": 8, "4": 18},
    },
    "truncated_cuboctahedron": {
        "display_name": "Truncated cuboctahedron",
        "vertices": 48,
        "edges": 72,
        "faces": 26,
        "face_histogram": {"4": 12, "6": 8, "8": 6},
    },
    "snub_cube": {
        "display_name": "Snub cube",
        "vertices": 24,
        "edges": 60,
        "faces": 38,
        "face_histogram": {"3": 32, "4": 6},
    },
    "icosidodecahedron": {
        "display_name": "Icosidodecahedron",
        "vertices": 30,
        "edges": 60,
        "faces": 32,
        "face_histogram": {"3": 20, "5": 12},
    },
    "truncated_dodecahedron": {
        "display_name": "Truncated dodecahedron",
        "vertices": 60,
        "edges": 90,
        "faces": 32,
        "face_histogram": {"3": 20, "10": 12},
    },
    "truncated_icosahedron": {
        "display_name": "Truncated icosahedron",
        "vertices": 60,
        "edges": 90,
        "faces": 32,
        "face_histogram": {"5": 12, "6": 20},
    },
    "rhombicosidodecahedron": {
        "display_name": "Rhombicosidodecahedron",
        "vertices": 60,
        "edges": 120,
        "faces": 62,
        "face_histogram": {"3": 20, "4": 30, "5": 12},
    },
    "truncated_icosidodecahedron": {
        "display_name": "Truncated icosidodecahedron",
        "vertices": 120,
        "edges": 180,
        "faces": 62,
        "face_histogram": {"4": 30, "6": 20, "10": 12},
    },
    "snub_dodecahedron": {
        "display_name": "Snub dodecahedron",
        "vertices": 60,
        "edges": 150,
        "faces": 92,
        "face_histogram": {"3": 80, "5": 12},
    },
}

SOLID_ORDER = tuple(EXPECTED_SOLIDS)
