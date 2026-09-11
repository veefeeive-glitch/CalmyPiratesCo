"""Publish exact reviewed R7 C06 bytes; keep R6 + its manifest recoverable.
Requires a clean live Blender session checked separately before invoking this.
No Unreal import, generated FBX replacement or GUI save is performed here.
"""
from pathlib import Path
import hashlib,json,shutil,gzip
ROOT=Path(__file__).resolve().parents[1]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
folder=ROOT/'SourceAssets/Generated/OceanFreighter'
candidate=folder/'R7-Cargo-C06/CalmyOceanFreighter-Cargo.blend'
evidence=ROOT/'Evidence/ShipTraversal/R7-Cargo-20260906'
sha='8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'
old='9ceafe214b04051c293734a81c31ab81e7daf109cfa73f82a8525a38ab759fdd'
gate=read(evidence/'acceptance.json');manifest=read(evidence/'evidence-manifest.json')
assert digest(candidate)==gate['source_sha256']==manifest['model_sha256']==sha
assert gate['status']=='SCOPED_CLEARANCE_PASS_WITH_RETAINED_EXPLORATORY_FAILURES'
assert gate['required_passed']==260 and gate['carry']['passed']==56 and gate['visual_count']==17
for name,expected in manifest['files'].items():assert digest(evidence/name)==expected,name
master=folder/'CalmyOceanFreighter.blend'
assert digest(master)==old,'Master changed: preserve user edits'
backup=folder/'Backup-20260906-R6';backup.mkdir(exist_ok=False)
shutil.copy2(master,backup/master.name);shutil.copy2(folder/'build_manifest.json',backup/'build_manifest.json')
assert digest(backup/master.name)==old
# Preserve fresh test runs and relevant source independently of ignored Saved.
history=evidence/'development-reports';history.mkdir()
for name in ('R6-Carry-20260906-03','R6-Carry-20260906-04','R6-Carry-20260906-05','R6-Carry-20260906-06',
             'R7-Carry-20260906-01','R7-C02-Carry-20260906-01','R7-C03-Carry-20260906-01','R7-C04-Carry-20260906-02'):
    shutil.copy2(ROOT/f'Saved/OceanPhysics/{name}.json',history/(name+'.json'))
for label in ('R6-Carry-20260906-06','R7-C02-Carry-20260906-01'):
    shutil.copy2(ROOT/f'Saved/Verification/{label}-Contacts/contacts.json',history/(label+'-contacts.json'))
network=evidence/'network';network.mkdir()
for label in ('20260906-014323-Carry','20260906-014410-Cargo','20260906-015122-Character','20260906-015247-Water'):
    shutil.copytree(ROOT/'Saved/Verification'/label,network/label)
shutil.copy2(ROOT/'Saved/Logs/Automation-20260906-024026.log',evidence/'automation.log')
for label in ('R7-C06-Carry-20260906-01','R7-C06-Traversal-20260906-01'):
    shutil.copy2(ROOT/f'Saved/OceanPhysics/{label}.log',evidence/(label+'.log'))
snapshot=evidence/'source-snapshot';snapshot.mkdir()
paths=['Source/CalmyPiratesCo/Private/Gameplay/CalmyCharacter.cpp','Source/CalmyPiratesCo/Private/Gameplay/CalmyCargo.cpp',
    'Source/CalmyPiratesCo/Private/Testing/CalmyOceanPhysicsProbe.cpp','Source/CalmyPiratesCo/Public/Testing/CalmyOceanPhysicsProbe.h',
    'Source/CalmyPiratesCo/Private/Tests/CarryClearanceTests.cpp','Scripts/verify_ocean_cargo_release.py','Scripts/test_ocean_cargo_release.py',
    'Scripts/run_ocean_physics.ps1','Scripts/export_ocean_physics.py','Scripts/verify_ocean_freighter_refit.py',
    'Scripts/build_ocean_cargo_candidate.py','Scripts/build_ocean_cargo_turns.py','Scripts/build_ocean_cargo_lobby.py',
    'Scripts/build_ocean_cargo_signs.py','Scripts/finish_ocean_cargo_sign_contrast.py','Scripts/finish_ocean_cargo_sign_mounts.py',
    'Scripts/render_cargo_candidate.py','Scripts/render_cargo_turns.py','Scripts/publish_ocean_cargo_r7.py']
for name in paths:
    dest=snapshot/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
ship=ROOT/'Saved/OceanPhysics/ship.json';assert digest(ship)==gate['collision_sha256']
with ship.open('rb') as src,gzip.open(evidence/'collision-input.json.gz','wb') as dest:shutil.copyfileobj(src,dest)
release=dict(model_sha256=sha,previous_sha256=old,backup=str(backup),
    module_sha256=digest(ROOT/'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll'),
    scope='Scoped Blender clearance increment only; WaterLab unchanged; no FBX/GLB/Unreal import; existing piping art findings open')
(evidence/'release.json').write_text(json.dumps(release,indent=2),encoding='utf-8')
manifest['files']={str(f.relative_to(evidence)):digest(f) for f in evidence.rglob('*') if f.is_file() and f.name!='evidence-manifest.json'}
(evidence/'evidence-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
# Only after evidence and backup are durable, replace exact master bytes.
assert digest(master)==old,'Master changed during archiving'
shutil.copy2(candidate,master);assert digest(master)==sha
build=read(folder/'build_manifest.json')
for key in ('preview','cutaway_preview','access_preview','bow_preview','stern_preview','mesh_objects','vertices','polygons','collections'):build.pop(key,None)
build.update(master_blend=str(master),revision='R7-Cargo-C06',status='Scoped cargo-clearance improvement; art and human acceptance incomplete',
    target_players='2-4 (game baseline; not a network acceptance claim)',build_mode='Hash-pinned R6 plus cargo candidates C01-C06; exact reviewed C06 bytes promoted',
    promoted_master_sha256=sha,physical_qa_report=str(evidence/'physical-report.json'),cargo_qa_report=str(evidence/'carry-report.json'),
    qa_report=str(evidence/'geometry-routes.json'),review_shots=str(evidence/'views/shots.json'),previews_generated=False,
    acceptance=str(evidence/'acceptance.json'),qa_scope=release['scope'])
(folder/'build_manifest.json').write_text(json.dumps(build,indent=2),encoding='utf-8')
print('R7_CARGO_PUBLISHED',sha,'R6_BACKUP',backup)
