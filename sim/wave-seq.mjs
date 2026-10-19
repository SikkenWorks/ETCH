/* Wave Sequencing V1.43. Uses existing per-step morph locks,
 * preserving every previously saved pattern and parameter lock. */
export function waveSeqPosition(step){return Math.round(100*(step.locks.morph??0));}
export function waveSeqMode(step){if(step.locks.morph===undefined)return 1;return step.locks.morph<=0?0:step.locks.morph>=1?2:1;}
export function waveSeqSet(step,pct){if(!Number.isFinite(pct)||pct<0||pct>100)throw Error('Invalid wave morph');step.locks.morph=pct/100;return step;}
export function waveSeqToggle(step,on){if(on){if(step.locks.morph===undefined)step.locks.morph=.5;}else delete step.locks.morph;return step;}
