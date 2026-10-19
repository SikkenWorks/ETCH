// ETCH keyboard zones: the main sound owns all notes not assigned to enabled groups.
export const emptyGroups=()=>[{enabled:false,patch:0,start:48,end:59},{enabled:false,patch:0,start:60,end:72}];
export function validateGroups(value){
 if(value==null)return emptyGroups();
 if(!Array.isArray(value)||value.length!==2)throw Error('Expected exactly two group slots');
 const groups=value.map(g=>{
  if(!g||typeof g!=='object'||typeof g.enabled!=='boolean'||!Number.isInteger(g.patch)||g.patch<0||g.patch>31||!Number.isInteger(g.start)||!Number.isInteger(g.end)||g.start<0||g.end>127||g.start>=g.end)throw Error('Invalid Group range or preset');
  return {enabled:g.enabled,patch:g.patch,start:g.start,end:g.end};
 });
 if(groups[0].enabled&&groups[1].enabled&&Math.max(groups[0].start,groups[1].start)<=Math.min(groups[0].end,groups[1].end))throw Error('Group ranges cannot overlap');
 return groups;
}
export function routeNote(note,groups){
 if(!Number.isInteger(note)||note<0||note>127)return -1;
 for(let i=0;i<2;i++)if(groups[i].enabled&&note>=groups[i].start&&note<=groups[i].end)return i+1;
 return 0;
}
export function noteLabel(n){return ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'][n%12]+(Math.floor(n/12)-1);}
