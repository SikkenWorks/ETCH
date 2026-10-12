#!/usr/bin/env python3
"""Read-only quality/format report for a PCM WAV recorded from ETCH USB audio.
Usage: py tools/u02_verify_windows_recording.py C:/recordings/etch-48k.wav
Does not validate the USB device/driver, by itself, or infer hardware success.
"""
import argparse, json, wave
from array import array
from pathlib import Path
import sys

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('wav',type=Path)
p.add_argument('--expected-rate',type=int,choices=(44100,48000),default=48000)
p.add_argument('--minimum-seconds',type=float,default=60)
a=p.parse_args()
with wave.open(str(a.wav),'rb') as w:
    rate=w.getframerate();channels=w.getnchannels();sw=w.getsampwidth();frames=w.getnframes()
    if sw!=2 or channels!=2:
        raise SystemExit('ERROR: expected 16-bit stereo PCM WAV; got %d channels, %d-bit'%(channels,sw*8))
    if frames<=0:
        raise SystemExit('ERROR: recording is empty')
    peak=[0,0];sum2=[0,0];nonzero=[0,0];clipped=[0,0]
    # Streaming analysis is safe even for long recordings.
    while True:
        b=w.readframes(16384)
        if not b:break
        samples=array('h');samples.frombytes(b)
        if sys.byteorder!='little':samples.byteswap()
        for i,x in enumerate(samples):
            ch=i%2
            ax=abs(int(x));peak[ch]=max(peak[ch],ax)
            sum2[ch]+=int(x)*int(x)
            if x:nonzero[ch]+=1
            if ax>=32767:clipped[ch]+=1
    sec=frames/rate
    result={'file':str(a.wav),'rate_hz':rate,'channels':channels,'duration_seconds':round(sec,3),
            'peak_ratio':[round(x/32768,6) for x in peak],
            'rms_ratio':[round((x/frames)**0.5/32768,6) for x in sum2],
            'nonzero_samples':nonzero,'clipped_samples':clipped,
            'format_and_duration_pass': rate==a.expected_rate and sec>=a.minimum_seconds and all(nonzero)}
    print(json.dumps(result,indent=2))
    if not result['format_and_duration_pass']:
        sys.exit(2)
    print('PASS format/duration/signal sanity only. Review playback and FM1 counters separately.')
