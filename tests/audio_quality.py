"""Optional spectral regression: Python 3 + NumPy and Node.js required."""
import json,subprocess,tempfile
from pathlib import Path
import numpy as np
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
    run=subprocess.run(['node',str(root/'tests/quality-render.mjs'),temp],check=True,capture_output=True,text=True)
    result={'alias_tests':[],'host_benchmark':json.loads(run.stdout)}
    for wave in ('SINE','SAW','SQUARE','SCRATCH'):
        for note in (84,96):
            fundamental=440*2**((note-69)/12)
            spectra={}
            for version in ('v01','v04'):
                x=np.fromfile(Path(temp)/f'{wave}-{version}-{note}.f32',dtype='<f4').astype(float)
                assert np.isfinite(x).all()
                power=np.abs(np.fft.rfft(x*np.hanning(len(x))))**2
                hz=np.fft.rfftfreq(len(x),1/44100)
                expected=np.zeros(len(hz),dtype=bool)
                # Mark a deliberately generous family of carrier/modulator sidebands as expected.
                for carrier_h in range(1,17):
                    for k in range(-30,31):
                        f=abs(carrier_h*fundamental+k*8*fundamental)
                        if f<22050: expected |= abs(hz-f)<12
                expected |= hz<20
                total=power.sum()
                spectra[version]=float(power[~expected].sum()/total) if total else 0.0
            improvement=10*np.log10(max(spectra['v01'],1e-30)/max(spectra['v04'],1e-30))
            # Sine must improve strongly. Hard-edged/drawn waves use an absolute residual-energy gate because
            # their legitimate high harmonics make a single old-vs-new ratio misleading.
            if wave=='SINE': assert improvement>20, (wave,note,improvement,spectra)
            else: assert spectra['v04']<0.02, (wave,note,spectra)
            result['alias_tests'].append({'wave':wave,'midi_note':note,'ratio':8,'phase_depth':1,'v01_unexpected_energy_fraction':spectra['v01'],'v04_unexpected_energy_fraction':spectra['v04'],'reduction_db':float(improvement)})
    print(json.dumps(result,indent=2))
