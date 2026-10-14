from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'firmware/fm1-native/bringup.c').read_text()
checks={
  'SEQ boot key': 'native_keyboard_mode=etch_keyboard_boot_requested(safe,fm1_in.buttons);',
  'no DSP audio initialization': 'if(!muted&&!native_keyboard_mode&&etch_recovery_may_run_audio(native_recovery_reason))',
  'no analog gate startup': 'if(!native_updating && !safe && !native_keyboard_mode)',
  'MIDI queue main-loop writer': 'etch_panel_midi_update(&native_panel_out,fm1_in.notes,native_keyboard_octave,',
  'MIDI queue interrupts masked': 'fm1_irq_off();\n     etch_panel_midi_update(&native_panel_out',
  'octave buttons': 'etch_keyboard_octave_step(native_keyboard_octave,press)',
  'no MIDI stale queue on reconnect': 'mo_r=mo_w;',
  'USB OTA maintained': 'if(usb.ota_req){',
  'boot-loop guard for audio-free mode': 'etch_boot_stable(&etch_boot,0);native_keyboard_boot_healthy=1u;',
  'version': '#define FELUCCA_ID "FM-1_1037"',
  'screen': 'label("MIDI KEYBOARD",12u,60u,0xffff);',
}
for key,needle in checks.items():
 assert needle in s,key
print('PASS V1.37 boot integration, USB-MIDI dispatch, mute and recovery wiring')
