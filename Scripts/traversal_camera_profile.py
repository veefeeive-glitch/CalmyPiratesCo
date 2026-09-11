"""Nominal inspection cameras. Gameplay CalcCamera remains authoritative.

Units are relative to capsule centre, not floor. At 88cm half height the current
bodycam is 124cm above flat ground; spring/fall/sweeps can change this in game.
Legacy profile exists only to reproduce historical R5 viewpoints.
"""

PROFILES = {
    'bodycam': {'forward_cm': 20, 'up_cm': 36, 'fov': 102},
    'steady': {'forward_cm': 20, 'up_cm': 36, 'fov': 92},
    'legacy-r5': {'forward_cm': 8, 'up_cm': 64, 'fov': 92},
}


def get_profile(name):
    if name not in PROFILES:
        raise ValueError('Unknown inspection camera: ' + name)
    return dict(PROFILES[name])
