import assert from 'node:assert/strict';
import {save,load,encode,decode,inspectStorage,STORAGE_KEYS,PROJECT_MEMORY_SLOTS,PATTERN_MEMORY_SLOTS} from '../sim/storage.mjs';
import {defaults,primitive} from '../sim/dsp.mjs';
import {makePerformance} from '../sim/performance.mjs';
import {BootSafetyMachine,BOOT_REASONS,makeBootRecord} from '../sim/boot-state.mjs';

function project(name='SAFE A'){
  const waves=Array(64).fill(null),patches=Array(32).fill(null);
  waves[0]={name:'SINE',points:primitive('SINE')};
  patches[0]={name,slots:[0,0,0],params:{...defaults},performance:makePerformance()};
  return {version:11,waves,patches,projects:Array(PROJECT_MEMORY_SLOTS).fill(null),patterns:Array(PATTERN_MEMORY_SLOTS).fill(null)};
}
class MemoryStorage{
  constructor(seed=[]){this.map=new Map(seed);this.failKey=null;this.partial=false;this.failRemoveKey=null;}
  getItem(k){return this.map.get(k)??null;}
  setItem(k,v){
    if(this.failKey===k){
      if(this.partial)this.map.set(k,String(v).slice(0,Math.max(1,Math.floor(String(v).length/3))));
      throw Error(`injected write failure: ${k}`);
    }
    this.map.set(k,String(v));
  }
  removeItem(k){if(this.failRemoveKey===k)throw Error(`injected remove failure: ${k}`);this.map.delete(k);}
}

const storage=new MemoryStorage();
const firstResult=save(storage,project('SAFE A'));assert(firstResult.verified&&firstResult.pendingCleared);
const first=storage.getItem(STORAGE_KEYS.current);
assert.equal(decode(first).patches[0].name,'SAFE A');
assert.deepEqual(inspectStorage(storage),{currentPresent:true,currentValid:true,previousPresent:false,previousValid:false,pendingPresent:false,pendingValid:false});

// 1. PENDING write fails before any committed state is touched.
const pendingFail=new MemoryStorage([[STORAGE_KEYS.current,first]]);pendingFail.failKey=STORAGE_KEYS.pending;pendingFail.partial=true;
assert.throws(()=>save(pendingFail,project('SAFE B')),/injected write failure/);
assert.equal(load(pendingFail).data.patches[0].name,'SAFE A');
assert.equal(inspectStorage(pendingFail).currentValid,true);

// 2. PREVIOUS rotation fails after a verified PENDING record. CURRENT remains authoritative.
const previousFail=new MemoryStorage([[STORAGE_KEYS.current,first]]);previousFail.failKey=STORAGE_KEYS.previous;
assert.throws(()=>save(previousFail,project('SAFE C')),/injected write failure/);
const previousFallback=load(previousFail);assert(previousFallback&&!previousFallback.recovered);assert.equal(previousFallback.data.patches[0].name,'SAFE A');
assert(previousFallback.orphanPending,'failed commit may leave diagnostic PENDING but must never boot it');

// 3. CURRENT write tears after PREVIOUS has been committed. Boot must fall back to PREVIOUS.
const currentFail=new MemoryStorage([[STORAGE_KEYS.current,first]]);currentFail.failKey=STORAGE_KEYS.current;currentFail.partial=true;
assert.throws(()=>save(currentFail,project('SAFE D')),/injected write failure/);
const recovered=load(currentFail);assert(recovered&&recovered.recovered,'torn current bank must fall back to previous');assert.equal(recovered.data.patches[0].name,'SAFE A');
const tornHealth=inspectStorage(currentFail);assert(tornHealth.previousValid&&!tornHealth.currentValid&&tornHealth.pendingValid);

// 4. Cleanup failure happens after a verified commit. The new CURRENT is valid; stale PENDING is diagnostic only.
const cleanupFail=new MemoryStorage([[STORAGE_KEYS.current,first]]);cleanupFail.failRemoveKey=STORAGE_KEYS.pending;
const cleanupResult=save(cleanupFail,project('SAFE E'));assert(cleanupResult.verified&&!cleanupResult.pendingCleared);
const cleanupLoaded=load(cleanupFail);assert.equal(cleanupLoaded.data.patches[0].name,'SAFE E');assert(cleanupLoaded.orphanPending);

// 5. Corruption and truncation never parse as success.
const good=encode(project('SAFE F'));
assert.throws(()=>decode(good.slice(0,-11)));
const wrapper=JSON.parse(good);wrapper.crc32='00000000';assert.throws(()=>decode(JSON.stringify(wrapper)),/Checksum or format mismatch/);

// 6. Externally corrupted CURRENT with intact PREVIOUS chooses PREVIOUS; valid PENDING is still ignored.
const pendingNew=encode(project('UNCOMMITTED'));
const storage3=new MemoryStorage([[STORAGE_KEYS.current,'{"broken":'],[STORAGE_KEYS.previous,first],[STORAGE_KEYS.pending,pendingNew]]);
const fallback=load(storage3);assert(fallback?.recovered);assert.equal(fallback.data.patches[0].name,'SAFE A');assert(fallback.orphanPending);

// 7. Boot state must fail closed for the exact spec-level triggers modeled on host.
const boot=new BootSafetyMachine({stableWindowMs:3000,maxEarlyFailures:2});
let state=boot.begin(makeBootRecord(),{storageCrcOk:false});assert.equal(state.reason,BOOT_REASONS.STORAGE_CRC);assert(state.outputMuted&&state.storageReadOnly&&state.updaterAvailable&&!state.loadLastPatch);
state=boot.begin({...makeBootRecord(),incomplete:true});assert.equal(state.reason,BOOT_REASONS.PREVIOUS_INCOMPLETE);
state=boot.begin({...makeBootRecord(),earlyFailures:2});assert.equal(state.reason,BOOT_REASONS.REPEATED_FAILURES);
state=boot.begin(makeBootRecord(),{audioInitOk:false});assert.equal(state.reason,BOOT_REASONS.AUDIO_INIT);assert(state.outputMuted);
state=boot.begin(makeBootRecord(),{uiInitOk:false});assert.equal(state.reason,BOOT_REASONS.UI_INIT);
state=boot.begin(makeBootRecord(),{octMinusHeld:true});assert.equal(state.reason,BOOT_REASONS.OCT_MINUS);
state=boot.begin(makeBootRecord());boot.failEarly(BOOT_REASONS.WATCHDOG);assert.equal(boot.snapshot().state.reason,BOOT_REASONS.WATCHDOG);assert.equal(boot.snapshot().record.earlyFailures,1);
state=boot.begin(makeBootRecord());assert.equal(state.mode,'NORMAL');assert(boot.markStable(3000));assert(boot.snapshot().record.bootGood);

console.log('R14 host fault injection: write-ahead journal boundaries, torn CURRENT recovery, orphan PENDING handling and boot-safe failure routing passed');
