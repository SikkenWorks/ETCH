#!/usr/bin/env python3
"""Build the ETCH V1.46 FX Resource Optimization .fwsc + matching WebInstaller.
Preserves exactly the reference bootheader/OTA binary. No flash I/O or hardware claim.
"""
import base64,hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_fwsc as fw
ROOT=Path(__file__).resolve().parents[1]
REFERENCE=ROOT/'compat/ETCH-V1.40-OTA-BASE-REFERENCE.fwsc'
APP=ROOT/'build/fm1-native-storage-updater-p05/app.bin'
OUT=ROOT/'release/ETCH-FM1-V1.46-FX-RESOURCE-OPTIMIZATION.fwsc'
INSTALLER=ROOT/'WebInstaller.html'
IDENTITY='FM-1_1046'
def sh(v):return hashlib.sha256(v).hexdigest()
def replace_one(src,pattern,replacement):
 src,n=re.subn(pattern,lambda m:replacement,src,count=1,flags=re.S)
 if n!=1:raise ValueError('Missing/ambiguous installer pattern '+pattern)
 return src

def main():
 report,errors=fw.verify_fwsc(REFERENCE)
 if errors:raise ValueError('Reference failed validation '+str(errors))
 app=APP.read_bytes()
 err=fw.validate_app_binary(app)
 if err:raise ValueError('Application validation error: '+str(err))
 assets=fw.extract_stock_assets(REFERENCE)
 old=fw.parse_ufw(REFERENCE.read_bytes())
 old_flash=next(e['data'] for e in old['entries'] if e['name']=='flash.bin')
 new_flash=fw.build_flash_image_from_assets(app,assets['uboot'],assets['cfg_tool'],assets['eq_cfg'],assets['key'],isd_config=assets['isd_config'],flash_header_payload=assets['flash_header_payload'])
 if len(old_flash)!=len(new_flash):raise ValueError('Flash layout unexpectedly changed')
 new_flash=old_flash[:0x4000]+new_flash[0x4000:]
 container=fw.build_fwsc(new_flash,assets['ota'],IDENTITY)
 OUT.parent.mkdir(exist_ok=True)
 OUT.write_bytes(container)
 v,err=fw.verify_fwsc(OUT)
 if err or v['product']!=IDENTITY:raise ValueError('Output not verified '+str(err))
 new=fw.parse_ufw(container)
 flash=next(e['data'] for e in new['entries'] if e['name']=='flash.bin')
 ota=next(e['data'] for e in new['entries'] if e['name']=='ota.bin')
 assert flash[:0x4000]==old_flash[:0x4000]
 assert ota==assets['ota']
 html=INSTALLER.read_text()
 html=replace_one(html,r'(<script id="firmware-embedded" type="application/octet-stream">)\s*[\s\S]*?(</script>)',r'\g<1>') if False else html
 encoded=base64.b64encode(container).decode('ascii')
 html=replace_one(html,r'(<script id="firmware-embedded" type="application/octet-stream">)\s*.*?(</script>)','<script id="firmware-embedded" type="application/octet-stream">\n'+encoded+'\n</script>')
 html=replace_one(html,r"HASH_ET: '[0-9a-f]{64}'",f"HASH_ET: '{sh(container)}'")
 # Hash of the exact 16 KiB protected bytes in this packaging. Prior static
 # WebInstaller hash was stale and rejected even a correct V1.41 package.
 protected_hash=sh(container[0:0]) if False else sh(old_flash[:0x4000])
 html=replace_one(html,r"KNOWN_HEAD_HASH: '[0-9a-f]{64}'",f"KNOWN_HEAD_HASH: '{protected_hash}'")
 html=replace_one(html,r"ID_ET: 'FM-1_\d+'",f"ID_ET: '{IDENTITY}'")
 html=replace_one(html,r'ET_LENGTH: \d+',f'ET_LENGTH: {len(container)}')
 html=html.replace('V1.44 PERFORMANCE MODE','V1.45 RESOURCE OPTIMIZATION')
 html=html.replace('V1.45: four macro knobs, two scenes, HOME+ARP PERFORMANCE. K1-scrollable HELP chapters; MONO default for new projects; GROUPS and wave sculpting.', 'V1.45: lossless DSP optimizations; GROUPS, all eight voices, Wave FX, performance macros and project data remain compatible. Hardware performance testing remains open.')
 html=html.replace('V1.44','V1.45')
 html=html.replace('FM-1_1044','FM-1_1046')
 html=html.replace('PERFORMANCE MODE · EXPERIMENTAL TEST','RESOURCE OPTIMIZATION · EXPERIMENTAL TEST')
 html=html.replace('v>=994&&v<=1044','v>=994&&v<=1045')
 html=html.replace('v>=994 && v<=1044','v>=994 && v<=1045')
 html=html.replace('V1.45: four macro knobs, two scenes, HOME+ARP PERFORMANCE. K1-scrollable HELP chapters; MONO default for new projects; GROUPS and wave sculpting.', 'V1.45: lossless DSP resource optimization; identical PCM across V1.44 audio stress tests. Four performance macros, GROUPS and full V1.44 features preserved.')
 html=html.replace('V1.43 WAVE SEQUENCING 2.0','V1.44 PERFORMANCE MODE')
 html=html.replace('Web Installer V1.44 WAVE SEQUENCING','Web Installer V1.44 PERFORMANCE MODE')
 html=html.replace('V1.43','V1.44')
 html=html.replace('V1.40 V1.44','V1.44')
 html=html.replace('GROUPS SETUP TEST','PERFORMANCE MODE')
 html=html.replace('SIKKENWORKS · V1.42 MOD MATRIX 2.0 · EXPERIMENTAL','SIKKENWORKS · V1.44 PERFORMANCE MODE · EXPERIMENTAL')
 html=html.replace('ETCH Web Installer V1.40 GROUPS TEST','ETCH Web Installer V1.44 PERFORMANCE MODE')
 html=html.replace('Install ETCH V1.40 GROUPS TEST first','Install a supported ETCH updater build first')
 html=html.replace('GROUPS TEST: native keyboard range editor and PJT29 group storage, with MIDI/FM1 key capture and PROJECT HELP. Group notes currently use the main sound; fully independent native timbres are still under development. Existing OTA/stock restore code is carried forward, not physically validated for this new build.', 'V1.44 PERFORMANCE MODE: HOME+ARP opens four assignable macros, SELECT toggles assignment, ALGORITHM selects A or B, REC captures and PLAY recalls. Scenes are saved in PJT31 projects. Physical FM1 installation is not yet verified.')
 html=html.replace('Native GROUPS screen, MIDI/panel START-END capture, ON/OFF, preset selection, PJT29 saving and Project HELP. IMPORTANT: GROUPS do not yet render independent preset waveforms on the FM1. This release is for testing the native setup/controls/storage. Installation and recovery on your hardware are unverified.', 'Full independent Group Wave A/B/C banks, per-Group synthesis settings and user waveform-FX with eight shared voices. START/END capture, ON/OFF, PJT29 and Project HELP included. Global reverb/delay/chorus stay on one shared FX bus. Physical FM1 verification still required.')
 html=html.replace('The complete ETCH engine is included, but it has not passed physical FM1 hardware qualification.', 'Includes the multitimbral GROUPS engine, but has not passed physical FM1 hardware qualification.')
 html=html.replace('This release is for testing the native setup/controls/storage.', 'This release is for experimental GROUPS audio testing.')
 html=html.replace('V1.44: per-step Wave A/B morph locks; HOME+FX GROUPS; HOME+SAVE PROJECTS; K1-scroll', 'V1.44: four macro knobs, two scenes, HOME+ARP PERFORMANCE. K1-scroll')
 html=html.replace("return v===989||(Number.isSafeInteger(v)&&v>=994&&v<=latest);", "return v===989||(Number.isSafeInteger(v)&&v>=994&&v<=1044)||v===1401||v===latest;")
 html=html.replace("return v===989||(Number.isSafeInteger(v)&&v>=994&&v<=1042)||v===1401||v===latest;", "return v===989||(Number.isSafeInteger(v)&&v>=994&&v<=1044)||v===1401||v===latest;")
 # Disable text selection and native context menu across the installer.
 html=html.replace('</body>', '''<script>document.addEventListener('contextmenu',e=>e.preventDefault());\n document.addEventListener('selectstart',e=>{if(!e.target.closest('input,textarea'))e.preventDefault()});</script>\n</body>''')
 html=html.replace('V1.45 RESOURCE OPTIMIZATION','V1.46 FX RESOURCE OPTIMIZATION')
 html=html.replace('V1.45','V1.46')
 html=html.replace('FM-1_1045','FM-1_1046')
 html=html.replace('lossless DSP optimizations; GROUPS, all eight voices, Wave FX, performance macros and project data remain compatible. Hardware performance testing remains open.', 'lossless global FX processing: skip inactive chorus/delay reads and reverb output mix while retaining all delay/reverb state. FM1 timing remains unverified.')
 html=html.replace('lossless DSP resource optimization; identical PCM across V1.44 audio stress tests.', 'lossless FX output optimization; PCM and FX-state parity with V1.45 reference.')
 INSTALLER.write_text(html)
 OUT_COPY=ROOT/'ETCH-WebInstaller-V1.46.html'
 OUT_COPY.write_bytes(INSTALLER.read_bytes())
 manifest={'build':'1.46 FX RESOURCE OPTIMIZATION','product':IDENTITY,'firmware':str(OUT.relative_to(ROOT)), 'fwsc_bytes':len(container), 'fwsc_sha256':sh(container), 'app_bytes':len(app),'app_sha256':sh(app),'bootheader_preserved':True,'ota_loader_preserved':True,'wave_seq_uses_existing_morph_locks':True,'home_fx_groups_chord':True,'home_save_projects_chord':True,'help_chapters':21,'default_voice':'MONO','groups_dirty_row_renderer':True,'matrix_routes':4,'matrix_sources':7,'matrix_signed_depth':True,'project_format':'PJT31','performance_macros':4,'performance_scenes':2,'home_arp_performance_chord':True,'native_wave_sculpt_five_modes':True,'sculpt_preview_preserves_source':True,'native_three_wavetable_banks':True,'eight_shared_voices':True,'global_fx_shared':True,'lossless_resource_optimization':True,'global_fx_zero_send_fastpaths':True,'global_fx_state_preserved':True,'per_block_part_cache':True,'filter_integer_q15':True,'physical_install_tested':False}
 (ROOT/'release/V1.46-RELEASE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 root_manifest={'version':'1.46','firmware_identity':IDENTITY,'fwsc_sha256':sh(container),'experimental_firmware_sha256':sh(container),'experimental_firmware_bytes':len(container),'firmware_file':str(OUT.relative_to(ROOT)),'application_sha256':sh(app),'source_project_version':'1.46','physical_install_tested':False,'features':{'performance_mode':'four assignable live macros and two recallable snapshots','project_format':'PJT31','help_chapters':21,'home_arp':'PERFORMANCE','home_fx':'GROUPS','home_save':'PROJECTS','new_project_default':'MONO'}}
 (ROOT/'RELEASE-MANIFEST.json').write_text(json.dumps(root_manifest,indent=2)+'\n')
 # Parser and installer are guaranteed to contain identical bytes.
 found=re.search(r'<script id="firmware-embedded" type="application/octet-stream">\s*(.*?)\s*</script>',html,re.S)
 assert found and base64.b64decode(found.group(1),validate=True)==container
 print(json.dumps(manifest,indent=2))
if __name__=='__main__':
    from build_audio_recovery_release import main as build_recovery
    build_recovery()  # V1.46 builder retired after audio hardware crash report
