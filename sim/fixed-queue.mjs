export class FixedQueue {
  constructor(capacity=4096){
    if(!Number.isInteger(capacity)||capacity<1)throw Error('Invalid fixed queue capacity');
    this.capacity=capacity;
    this.items=new Array(capacity);
    this.count=0;
    this.dropped=0;
  }
  get size(){return this.count;}
  get full(){return this.count>=this.capacity;}
  push(value){
    if(this.count>=this.capacity){this.dropped++;return false;}
    this.items[this.count++]=value;
    return true;
  }
  clear(){
    for(let i=0;i<this.count;i++)this.items[i]=undefined;
    this.count=0;
  }
  drainBefore(limit,consumer){
    let write=0,drained=0;
    for(let read=0;read<this.count;read++){
      const item=this.items[read];
      if(item.time<limit){consumer(item);drained++;}
      else{if(write!==read)this.items[write]=item;write++;}
    }
    for(let i=write;i<this.count;i++)this.items[i]=undefined;
    this.count=write;
    return drained;
  }
  snapshot(){return this.items.slice(0,this.count);}
}
