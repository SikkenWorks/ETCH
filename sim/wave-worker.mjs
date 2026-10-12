import {compileWave} from './dsp.mjs';
const cache=new Map();
self.onmessage=({data})=>{
 try{
  const bank=data.waves.map(item=>{const key=JSON.stringify(item);if(cache.has(key))return cache.get(key);const wave=compileWave(item.points,item.fx);if(cache.size>=8)cache.delete(cache.keys().next().value);cache.set(key,wave);return wave;});
  self.postMessage({id:data.id,bank});
 }catch(e){self.postMessage({id:data.id,error:e.message});}
};
