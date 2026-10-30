#!/usr/bin/env python3
"""Native host validation. No device access; optional .fwsc input is read only."""
from pathlib import Path
import argparse,os,subprocess,tempfile,signal,sys,json,time
from concurrent.futures import ThreadPoolExecutor,as_completed
ap=argparse.ArgumentParser();ap.add_argument('--only',type=str,help='comma-separated test names for bounded targeted validation');ap.add_argument('--reference',type=Path,help='read-only OTA simulation');ap.add_argument('--timeout',type=int,default=35,help='maximum seconds per test or compile (default 35)');ap.add_argument('--jobs',type=int,default=3);ap.add_argument('--report',type=Path);ap.add_argument('--self-test-hang',action='store_true');a=ap.parse_args()
r=Path(__file__).resolve().parents[1];n=r/'firmware/fm1-native'
cases={'organic_v123':['audio_core.c','native_performance.c','native_fx.c','native_project_codec.c','native_pattern_bank.c'], 'dsp':['audio_core.c','native_performance.c','native_fx.c'],'audio':['audio_core.c','native_performance.c','native_fx.c'],'dma':['audio_core.c','native_performance.c','native_fx.c','native_wave_fx.c','wave_editor.c','native_panel_midi.c'],'editor':['audio_core.c','native_performance.c','native_fx.c','native_wave_fx.c','wave_editor.c'],'store':['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c'],'project':['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c'],'midi':['audio_core.c','native_performance.c','native_fx.c'],'stage0_boot':[],'irq_bridge':[],'audio_timing':[],'usb':[],'usb_audio':[],'usb_audio_soak':[],'usb_audio_48k':[],'usb_audio_dma_shift':[],'digest':['native_digest.c'],'fx':['native_fx.c'],'performance':['audio_core.c','native_performance.c','native_fx.c'],'poly':['audio_core.c','native_performance.c','native_fx.c'],'tools':['wave_editor.c','native_wave_fx.c'],'wavefx':['wave_editor.c','native_wave_fx.c'],'factory':['native_factory.c'],'panel_midi':['native_panel_midi.c'],'perf_midi':['native_performance_midi.c'],'pattern_bank':['native_pattern_bank.c','native_performance.c'],'upgrade':['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c'],'layers':['native_performance.c'],'fx_advanced':['native_fx.c'],'fx_stereo':['native_fx.c'],'fx_rc_guard':['native_fx.c'],'p04':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'wave_compression':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'v9_to_v10':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'v8_to_v9':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'harmony_bake':['native_performance.c'],'guide':['native_performance.c'],'clock_accuracy':['native_performance_midi.c'],'midi_soak':['native_performance_midi.c'],'note_editor':['native_performance.c'],'voice_steal':['audio_core.c','native_performance.c','native_fx.c'],'midi_instances':['audio_core.c','native_performance.c','native_fx.c'],'arp_flags':['native_performance.c'],'arp_gate':['audio_core.c','native_performance.c','native_fx.c'],'arp_program':['native_performance.c'],'arp_custom_audio':['audio_core.c','native_performance.c','native_fx.c'],'seq_ui':['native_seq_ui.c','native_performance.c'],'hold_links':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_seq_ui.c','native_fx.c'],'hold_audio':['audio_core.c','native_performance.c','native_fx.c'],'v6_to_v7':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'ui_wave':[],'xy32':['wave_editor.c','native_wave_fx.c'],'ui_navigation':[],'recovery_policy':[],'capacity_v049':['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'recovery_storage_v049':['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],'diagnostics':[],'panel_capture':[],'fx_calibration':['native_fx.c'],'patch_swap':['native_pattern_bank.c','native_performance.c','audio_core.c','native_fx.c','wave_editor.c','native_wave_fx.c'], 'overload':['audio_core.c','native_performance.c','native_fx.c'], 'advanced_steal':['audio_core.c','native_performance.c','native_fx.c'], 'scope_ui':[], 'harmonic_edit':['wave_editor.c','native_wave_fx.c'], 'music_labels':[],'keylock_led':['native_performance.c']}
cases['v188_waveform_engine']=['wave_editor.c','native_wave_fx.c']
cases['riff_v128']=['native_performance.c']
cases['riff_codec_v128']=['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c']
cases['matrix_v126']=['audio_core.c','native_performance.c','native_fx.c','native_project_codec.c','native_pattern_bank.c']
cases['living_v125']=['audio_core.c','native_performance.c','native_fx.c','native_project_codec.c','native_pattern_bank.c']
cases['spectral_v124']=['wave_editor.c','native_wave_fx.c']
cases['riff_bpm_v124']=['audio_core.c','native_performance.c','native_fx.c']
cases['motion_v113']=['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c']
cases['audio_v121']=['audio_core.c','native_performance.c','native_fx.c']
cases['v111_tempo']=['audio_core.c','native_performance.c','native_fx.c']
cases['v111_silence']=['audio_core.c','native_performance.c','native_fx.c']
cases['professional_v110']=['native_fx.c','native_project_codec.c','native_pattern_bank.c','native_performance.c']
cases['fx_v102']=['native_fx.c','native_project_codec.c','native_pattern_bank.c','native_performance.c']
cases['four_fx_v106']=['native_fx.c','native_project_codec.c','native_pattern_bank.c','native_performance.c']
cases['v107_wave_fx_mod']=['audio_core.c','native_fx.c','native_project_codec.c','native_pattern_bank.c','native_performance.c']
cases['seq_parity']=[]
cases['encoder_steps']=[]
cases['bpm_restore']=['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c']
cases['fx_space_v122']=['native_fx.c']
cases['fx_pcm_v174']=['native_fx.c']
cases['v175_shared_undo']=['wave_editor.c','native_wave_fx.c']
cases['organic_smooth_v176']=[]
cases['startup_pop']=[]
cases['home_click_v105']=['audio_core.c','native_performance.c','native_fx.c']
cases['ring_eq_v096']=['native_fx.c','native_project_codec.c','native_pattern_bank.c','native_performance.c']
cases['mono_v097']=['audio_core.c','native_performance.c','native_fx.c','native_project_codec.c','native_pattern_bank.c']
cases['mono_advanced_v100']=['audio_core.c','native_performance.c','native_fx.c']
cases['preset_v099']=['native_fx.c','native_project_codec.c','native_pattern_bank.c','native_performance.c']
cases['preset_ramp_v108']=[]
cases['v130_controls']=[]
cases['v130_preset_codec']=['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c']
cases['v131_preset_codec']=['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c']
cases['v140_pjt28']=['native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c']
cases['v131_living']=[]
cases['v132_mip_cache']=['native_fx.c','native_performance.c']
cases['v132_poly_stress']=['audio_core.c','native_performance.c','native_fx.c']
cases['v132_spectral_soak']=['audio_core.c','native_performance.c','native_fx.c','wave_editor.c','native_wave_fx.c']
cases['v131_spectral_controls']=['wave_editor.c','native_wave_fx.c']
cases['v133_poly_menu']=[]
cases['v133_poly_bypass']=['audio_core.c','native_performance.c','native_fx.c']
cases['v135_poly_guard']=['audio_core.c','native_performance.c','native_fx.c']
cases['v135_poly_menu']=[]
cases['v136_riff']=['audio_core.c','native_performance.c','native_fx.c']
cases['keyboard_v137']=['native_panel_midi.c']
if a.reference:cases['ota']=['native_digest.c']

# Each test is a separate process group. Timeouts kill children as well as the
# parent, so no lingering process can stall a future build or test run.
def bounded(command, timeout, cwd=None, env=None):
    start=time.monotonic()
    proc=subprocess.Popen(command,cwd=cwd,env=env,stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,text=True,start_new_session=True)
    try:
        out,err=proc.communicate(timeout=timeout)
        return proc.returncode,out,err,time.monotonic()-start
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid,signal.SIGKILL)
        out,err=proc.communicate()
        return 124,out,err+'\nTIMEOUT; process group killed',time.monotonic()-start

if a.self_test_hang:
    rc,out,err,_=bounded([sys.executable,'-c','import time; time.sleep(3600)'],1)
    assert rc==124 and 'TIMEOUT' in err
    print('PASS runner timeouts kill hanging subprocesses',flush=True)
    sys.exit(0)

# Full regression includes the previously separate extended-flash and OTA cases.
extra={
    'p04_extended':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_P04_EXPANDED=1']),
    'pattern_archive_24k':(['native_pattern_bank.c','native_performance.c'],['-DETCH_P04_EXPANDED=1']),
    'p05_legacy':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_P04_EXPANDED=0','-DETCH_P05_MULTI=1']),
    'global_waves':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_P05_MULTI=1','-DETCH_P04_EXPANDED=1']),
    'global_waves_legacy':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_P05_MULTI=1','-DETCH_P04_EXPANDED=0']),
    'p05':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_P04_EXPANDED=1','-DETCH_P05_MULTI=1']),
    'spare_store_v085':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_STORE_SPARE_ENABLED=1','-DETCH_P05_MULTI=1']),
    'storage_takeover_v086':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_STORE_SPARE_ENABLED=1','-DETCH_STORE_OWNED_ENABLED=1','-DETCH_P05_MULTI=1']),
    'v095_projects':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_STORE_OWNED_ENABLED=1','-DETCH_STORE_SPARE_ENABLED=0','-DETCH_P05_MULTI=1']),
    'preset_flash_v102':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_STORE_OWNED_ENABLED=1','-DETCH_STORE_SPARE_ENABLED=0','-DETCH_P05_MULTI=1']),
    'preset_flash_v099':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_STORE_OWNED_ENABLED=1','-DETCH_STORE_SPARE_ENABLED=0','-DETCH_P05_MULTI=1']),
    'etch_storage_v089':(['wave_store.c','native_project_codec.c','native_pattern_bank.c','native_performance.c','native_fx.c'],['-DETCH_STORE_OWNED_ENABLED=1','-DETCH_STORE_SPARE_ENABLED=0','-DETCH_P05_MULTI=1']),
    'master_gain_v089':(['audio_core.c','native_performance.c','native_fx.c'],[]),
    'seq_actual_v086':([],[]),
    'fx_signal_v085':(['native_fx.c'],[]),
    'ota_target_0':([],['-DT_OTA=0']),
    'ota_target_1':([],['-DT_OTA=1']),
}
tasks=[(key,src,[]) for key,src in cases.items()]
tasks.extend((key,src,flags) for key,(src,flags) in extra.items())
if a.only:
 wanted=set(a.only.split(","));tasks=[task for task in tasks if task[0] in wanted]
 if {item[0] for item in tasks}!=wanted:raise SystemExit("Unknown requested test: "+str(wanted-{item[0] for item in tasks}))
def test_one(item,work):
    name,sources,flags=item
    filename=('ota_target' if name.startswith('ota_target_') else 'p05' if name=='p05_legacy' else 'global_waves' if name=='global_waves_legacy' else name)
    out=work/name
    cmd=['gcc','-std=c11','-O1','-g','-fsanitize=address,undefined',
         '-ffunction-sections','-Wl,--gc-sections',*flags,
         str(r/f'tests/test_native_{filename}.c'),
         *[str(n/x) for x in sources],'-o',str(out)]
    code,stdout,stderr,compile_time=bounded(cmd,a.timeout)
    if code:return {'name':name,'status':'TIMEOUT' if code==124 else 'COMPILE FAIL','seconds':compile_time,'detail':stderr[-1000:]}
    command=[str(out)]+([str(a.reference.resolve())] if name=='ota' else [])
    code,stdout,stderr,run_time=bounded(command,a.timeout,cwd=r,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
    return {'name':name,'status':'PASS' if code==0 else 'TIMEOUT' if code==124 else 'FAIL',
            'seconds':round(compile_time+run_time,2),
            'detail':(stdout if code==0 else stderr)[-1000:]}

results=[]
with tempfile.TemporaryDirectory(prefix='etch-native-bounded-') as tmp:
    work=Path(tmp)
    with ThreadPoolExecutor(max_workers=max(1,min(a.jobs,5))) as workers:
        pending={workers.submit(test_one,item,work):item[0] for item in tasks}
        for task in as_completed(pending):
            try:result=task.result()
            except Exception as exc:result={'name':pending[task],'status':'FAIL','detail':repr(exc)}
            results.append(result)
            print(f"{result['status']} {result['name']} ({result.get('seconds',0)}s)",flush=True)
            if result['status']!='PASS':print(result.get('detail','')[-650:],flush=True)

passes=sum(x['status']=='PASS' for x in results)
print(f'RESULT {passes}/{len(results)} passed, {len(results)-passes} failed; all test processes exited',flush=True)
if a.report:
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps({'passed':passes,'total':len(results),'cases':sorted(results,key=lambda x:x['name'])},indent=2)+'\n')
sys.exit(int(passes!=len(results)))
