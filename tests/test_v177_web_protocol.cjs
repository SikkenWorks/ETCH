const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(require('node:path').join(__dirname,'../ETCH-Project-Preset-Manager.html'),'utf8');
const js=html.split('<script>')[1].split('</script>')[0];
const fields={};const document={addEventListener(){},getElementById(id){return fields[id]??=( {disabled:false, value:0,textContent:'',addEventListener(){}} )}};
const ctx=vm.createContext({document,navigator:{},console,TextDecoder,Uint8Array,Date,Array,Math,Promise,Error,Number,Boolean,JSON,setTimeout,clearTimeout});
vm.runInContext(js,ctx);
vm.runInContext(`
for(let cmd of [0x11,0x60,0x61,0x62,0x63,0x64])for(let len of [0,1,2,3,23,24,27,32]){
 const payload=Array.from({length:len},(_,i)=>(i*29+103)&255);
 const encoded=packet(cmd,payload),got=decode(encoded);
 if(!got||got.cmd!==cmd||got.data.length!==payload.length||got.data.some((n,i)=>n!==payload[i]))throw Error('bad SysEx roundtrip');
 const bad=encoded.slice();bad[bad.length-2]=(bad[bad.length-2]^1)&127;
 if(decode(bad)!==null)throw Error('damaged checksum was accepted');
}
if(crc32(new Uint8Array([49,50,51,52,53,54,55,56,57]))!==0xcbf43926)throw Error('bad standard CRC32');
`,ctx);
console.log('PASS 48 SysEx fixture roundtrips and 48 corrupt-checksum rejections; standard CRC32 vector');

// The device replies to snapshot READ (0x61) with DATA (0x62), not 0x61.
// Exercise the live request matcher without real hardware to detect regressions.
(async()=>{
 const promise=vm.runInContext(`(()=>{output={send:()=>{}};return wait(0x61,[0,0],a=>a.length===3,1500,0x62)})()`,ctx);
 const result=vm.runInContext(`(()=>{receive({data:packet(0x62,[0,0,0])});return true})()`,ctx);
 assert.equal(result,true);
 assert.equal((await promise).length,3);
 console.log('PASS live USB-MIDI READ 0x61 -> DATA 0x62 response correlation');
})().catch(e=>{console.error(e);process.exitCode=1});
