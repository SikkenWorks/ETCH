/* V1.44 performance macros: project-scoped, additive and backward-compatible. */
export const PERFORMANCE_TARGETS=['morph','depth','cutoff','drive','reverb'];
export function emptyPerformanceMacros(){return {targets:[0,1,2,3],scenes:[null,null]};}
export function validatePerformanceMacros(input){
 if(input==null)return emptyPerformanceMacros();
 if(!input||typeof input!=='object'||!Array.isArray(input.targets)||input.targets.length!==4||
 !input.targets.every(v=>Number.isInteger(v)&&v>=0&&v<PERFORMANCE_TARGETS.length)||
 !Array.isArray(input.scenes)||input.scenes.length!==2)throw Error('Invalid performance macros');
 const scenes=input.scenes.map(scene=>{
  if(scene==null)return null;
  if(!Array.isArray(scene)||scene.length!==4||
     !scene.every(v=>Number.isInteger(v)&&v>=0&&v<=100))throw Error('Invalid performance snapshot');
  return [...scene];
 });
 return {targets:[...input.targets],scenes};
}
const LIMITS=[[0,1],[0,1],[20,18000],[0,1],[0,1]];
export function readMacro(params,target){
 if(!Number.isInteger(target)||target<0||target>=5)throw Error('Invalid performance target');
 const key=PERFORMANCE_TARGETS[target],[min,max]=LIMITS[target];
 return Math.max(0,Math.min(100,Math.round(100*((params[key]??min)-min)/(max-min))));
}
export function writeMacro(params,target,value){
 if(!Number.isInteger(value)||value<0||value>100)throw Error('Invalid macro position');
 if(!Number.isInteger(target)||target<0||target>=5)throw Error('Invalid performance target');
 const key=PERFORMANCE_TARGETS[target],[min,max]=LIMITS[target];
 const next=min+(max-min)*value/100;
 return {...params,[key]:target===2?Math.round(next):next};
}
export function captureScene(macros,params,scene){
 const m=validatePerformanceMacros(macros);if(![0,1].includes(scene))throw Error('Invalid scene');
 m.scenes[scene]=m.targets.map(i=>readMacro(params,i));return m;
}
export function recallScene(macros,params,scene){
 const m=validatePerformanceMacros(macros);if(![0,1].includes(scene)||!m.scenes[scene])return null;
 let next={...params};for(let i=0;i<4;i++)next=writeMacro(next,m.targets[i],m.scenes[scene][i]);
 return next;
}
