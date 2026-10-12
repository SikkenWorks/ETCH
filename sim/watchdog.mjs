export class WatchdogSupervisor{
 constructor({timeoutMs=500}={}){
  if(!Number.isInteger(timeoutMs)||timeoutMs<50)throw Error('Invalid watchdog timeout');
  this.state={timeoutMs,lastKickMs:0,crashCount:0,armed:false,tripped:false};
 }
 arm(nowMs=0){if(!Number.isFinite(nowMs))throw Error('Invalid watchdog time');this.state.lastKickMs=nowMs>>>0;this.state.armed=true;this.state.tripped=false;}
 kick(nowMs=0){if(!this.state.armed)return false;if(!Number.isFinite(nowMs))return false;this.state.lastKickMs=nowMs>>>0;this.state.tripped=false;return true;}
 poll(nowMs=0){if(!this.state.armed||this.state.tripped)return this.state.tripped;if(!Number.isFinite(nowMs))return false;const now=nowMs>>>0,elapsed=(now-this.state.lastKickMs)>>>0;if(elapsed<this.state.timeoutMs)return false;this.state.tripped=true;this.state.crashCount=Math.min(65535,this.state.crashCount+1);return true;}
 disarm(){this.state.armed=false;this.state.tripped=false;}
 clearCrashCount(){this.state.crashCount=0;}
 snapshot(){return {...this.state};}
}
