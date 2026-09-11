"""Original quiet, loop-safe wind and hull resonance. No downloaded recordings."""
from pathlib import Path
import math,random,wave,struct,json
out=Path(__file__).resolve().parents[1]/'SourceAssets/Generated/NightwatchAudioR1'
assert not out.exists();out.mkdir(parents=True)
rate=22050;duration=32;count=rate*duration
report={}
for name in ('Wind','Hull'):
    rng=random.Random(508);low=0.;samples=[]
    for i in range(count):
        t=i/rate;low=.985*low+.015*rng.uniform(-1,1)
        if name=='Wind':
            value=low*(.7+.3*math.sin(2*math.pi*t/16))+.015*math.sin(2*math.pi*71*t)*(.5+.5*math.sin(2*math.pi*t/32))
        else:
            pulse=max(0,math.sin(2*math.pi*t/16))**8
            value=.03*math.sin(2*math.pi*43*t)+pulse*(.045*math.sin(2*math.pi*117*t+1.8*math.sin(2*math.pi*t/8))+.02*math.sin(2*math.pi*173*t))
        fade=min(1,t/.5,(duration-t)/.5);samples.append(value*max(0,fade))
    peak=max(abs(x) for x in samples);gain=.22/peak
    with wave.open(str(out/(name+'.wav')),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate);w.writeframes(struct.pack('<'+'h'*count,*(round(x*gain*32767) for x in samples)))
    report[name]={'seconds':duration,'sample_rate':rate,'peak':.22,'rms':(sum((x*gain)**2 for x in samples)/count)**.5,'original_synthesis':True}
(out/'audio_manifest.json').write_text(json.dumps(report,indent=2))
print('NIGHTWATCH_AUDIO_PASS',report)
