"""Run with python3 temporal_scene_editor_test.py <editor> [scene-binarizer]."""
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

repo = Path(__file__).resolve().parents[3]
editor = str(Path(sys.argv[1]).resolve())
base = repo / 'project_data/scenes/gravity_test.meta_ilenscene'
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    scenes = root / 'project_data/scenes'
    scenes.mkdir(parents=True)
    layout = scenes / 'test.txt'
    output = layout.with_suffix('.meta_ilenscene')
    def run(text, answer='', use_base=False):
        layout.write_text(text)
        args = [editor, str(layout)]
        if use_base:
            args += ['--base-scene', str(base)]
        return subprocess.run(args, input=answer, text=True, capture_output=True)

    text = 'Level_Name: Test\nGrid_Size: 3x2\n[G][P][G]\n[][ G ][]\n'
    result = run(text, use_base=True)
    assert result.returncode == 0, result.stderr
    scene = json.loads(output.read_text())
    entities = scene['entities']
    grounds = [e['components'] for e in entities if e['components'].get('ScriptingBridgeComponent', {}).get('scriptName') == 'ilen::game::Ground' and 'RenderComponent' in e['components']]
    assert len(grounds) == 3
    assert [g['TransformComponent']['position'] for g in grounds] == [[-3, 3, 100], [3, 3, 100], [0, 0, 100]]
    for g in grounds:
        assert g['TransformComponent']['scale'] == [3, 3, 3]
        assert g['RenderComponent'] == {'mainTexID': 36, 'defaultSpriteInTextureID': 3}
        assert g['Rigidbody2D'] == {'gravityScale': 1, 'isStatic': True}
        assert g['BoxCollider2D'] == {}
        assert g['EntityMetadataComponent']['activeStatus'] is True
        assert g['EntityMetadataComponent']['tags'] == g['EntityMetadataComponent']['layers'] == 1
    ids = [e['components']['EntityMetadataComponent']['uuid'] for e in entities]
    assert len(set(ids)) == len(ids) and all(0 < i < 2**64 for i in ids)
    original = json.loads(base.read_text())
    def check_player(entities):
        markers = [e['components'] for e in entities if e['components']['EntityMetadataComponent']['name'] == 'Player Spawn']
        assert len(markers) == 1
        marker = markers[0]
        assert set(marker) == {'EntityMetadataComponent', 'TransformComponent', 'ScriptingBridgeComponent'}
        assert marker['TransformComponent'] == {'position': [0, 3, 10]}
        assert marker['ScriptingBridgeComponent'] == {'scriptName': 'ilen::game::Ground'}
        assert marker['EntityMetadataComponent']['activeStatus'] is True
        assert marker['EntityMetadataComponent']['tags'] == marker['EntityMetadataComponent']['layers'] == 0
        assert not any('CameraComponent' in e['components'] for e in entities)
        assert not any(e['components'].get('ScriptingBridgeComponent', {}).get('scriptName') in ('ilen::game::Cube', 'ilen::game::GravityCamera') for e in entities)
    check_player(entities)
    assert len(entities) == 5  # Pointer, three Grounds and the spawn marker.
    pointer = next(e for e in entities if e['components']['EntityMetadataComponent']['name'] == 'Pointer Pos')
    expected_pointer = original['entities'][-1]
    pointer_copy = json.loads(json.dumps(pointer))
    pointer_copy['components']['EntityMetadataComponent']['uuid'] = expected_pointer['components']['EntityMetadataComponent']['uuid']
    assert pointer_copy == expected_pointer
    if len(sys.argv) > 2:
        manifests = root / 'project_data/manifests'
        manifests.mkdir()
        shutil.copy(repo / 'project_data/manifests/components.meta_ilenschema', manifests)
        result = subprocess.run([str(Path(sys.argv[2]).resolve()), '--ilen-folder', str(root)], capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
        assert 'Ignorado:' not in result.stdout + result.stderr
        binary = (root / '.ilen-bin-data/scenes/test.ilenscene').read_bytes()
        magic, version, count, components = struct.unpack_from('<20sIII', binary)
        assert magic.startswith(b'ENECS_NELI') and version == 1 and count == len(ids)
        assert components == len(set().union(*(e['components'] for e in entities)))
        assert list(struct.unpack_from('<' + 'Q' * count, binary, 32)) == ids

    result = run(text, 's\n')  # P also works without a base scene.
    assert result.returncode == 0, result.stderr
    standalone = json.loads(output.read_text())['entities']
    assert len(standalone) == 4
    check_player(standalone)
    previous = output.read_bytes()
    duplicate = 'Level_Name: Test\nGrid_Size: 2x1\n[P][P]\n'
    result = run(duplicate, 's\n')
    assert result.returncode != 0 and 'Solo se permite un marcador' in result.stderr
    assert output.read_bytes() == previous and layout.read_text() == duplicate
    bad = 'Level_Name: Test\nGrid_Size: 2x2\n[P]\n'
    assert run(bad, 'n\n').returncode != 0
    assert layout.read_text() == bad and output.read_bytes() == previous
    assert run(text, 'n\n').returncode != 0 and output.read_bytes() == previous
    assert run('Level_Name: Test\nGrid_Size: 1x1\n[X]\n').returncode != 0
    assert output.read_bytes() == previous
    assert run(bad, 's\ns\n').returncode == 0
    assert layout.read_text().endswith('[][]\n[][]\n')
    assert json.loads(output.read_text())['entities'] == []
print('Passed: Ground/spawn marker mapping, duplicate markers, UUIDs, base scene, cancellation, reset, unknown values and optional engine binarization.')
