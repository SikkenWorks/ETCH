export const BOOT_REASONS=Object.freeze({
  NORMAL:'NORMAL',
  OCT_MINUS:'OCT_MINUS',
  PREVIOUS_INCOMPLETE:'PREVIOUS_INCOMPLETE',
  STORAGE_CRC:'STORAGE_CRC',
  AUDIO_INIT:'AUDIO_INIT',
  UI_INIT:'UI_INIT',
  REPEATED_FAILURES:'REPEATED_FAILURES',
  WATCHDOG:'WATCHDOG',
  INPUT_OVERFLOW:'INPUT_OVERFLOW'
});

export function makeBootRecord(){return {incomplete:false,earlyFailures:0,lastReason:BOOT_REASONS.NORMAL,bootGood:true};}

export class BootSafetyMachine {
  constructor({stableWindowMs=3000,maxEarlyFailures=2}={}){
    if(!Number.isFinite(stableWindowMs)||stableWindowMs<100)throw Error('Invalid stable boot window');
    if(!Number.isInteger(maxEarlyFailures)||maxEarlyFailures<1)throw Error('Invalid early failure limit');
    this.stableWindowMs=stableWindowMs;
    this.maxEarlyFailures=maxEarlyFailures;
    this.record=makeBootRecord();
    this.state=this.#safeState(BOOT_REASONS.NORMAL,false);
  }
  #safeState(reason,readOnly=true){return {mode:'SAFE_MODE',reason,outputMuted:true,storageReadOnly:readOnly,loadLastPatch:false,updaterAvailable:true,bootInProgress:false,bootGood:false};}
  begin(record=makeBootRecord(),inputs={}){
    this.record={...makeBootRecord(),...record};
    const i={octMinusHeld:false,storageCrcOk:true,audioInitOk:true,uiInitOk:true,...inputs};
    let reason=null,readOnly=true;
    if(i.octMinusHeld)reason=BOOT_REASONS.OCT_MINUS;
    else if(this.record.incomplete)reason=BOOT_REASONS.PREVIOUS_INCOMPLETE;
    else if(this.record.earlyFailures>=this.maxEarlyFailures)reason=BOOT_REASONS.REPEATED_FAILURES;
    else if(!i.storageCrcOk){reason=BOOT_REASONS.STORAGE_CRC;readOnly=true;}
    else if(!i.audioInitOk)reason=BOOT_REASONS.AUDIO_INIT;
    else if(!i.uiInitOk)reason=BOOT_REASONS.UI_INIT;
    if(reason){
      this.record.lastReason=reason;
      this.record.bootGood=false;
      this.state=this.#safeState(reason,readOnly);
      return this.state;
    }
    this.record.incomplete=true;
    this.record.bootGood=false;
    this.record.lastReason=BOOT_REASONS.NORMAL;
    this.state={mode:'NORMAL',reason:BOOT_REASONS.NORMAL,outputMuted:false,storageReadOnly:false,loadLastPatch:true,updaterAvailable:true,bootInProgress:true,bootGood:false};
    return this.state;
  }
  markStable(runtimeMs){
    if(this.state.mode!=='NORMAL'||!this.state.bootInProgress)return false;
    if(!Number.isFinite(runtimeMs)||runtimeMs<this.stableWindowMs)return false;
    this.record.incomplete=false;
    this.record.earlyFailures=0;
    this.record.bootGood=true;
    this.state={...this.state,bootInProgress:false,bootGood:true};
    return true;
  }
  failEarly(reason=BOOT_REASONS.AUDIO_INIT){
    if(!Object.values(BOOT_REASONS).includes(reason)||reason===BOOT_REASONS.NORMAL)reason=BOOT_REASONS.AUDIO_INIT;
    this.record.incomplete=true;
    this.record.earlyFailures=Math.min(255,(this.record.earlyFailures||0)+1);
    this.record.bootGood=false;
    this.record.lastReason=reason;
    this.state=this.#safeState(reason,true);
    return this.state;
  }
  snapshot(){return {record:{...this.record},state:{...this.state}};}
}
