// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.lock[i], "bcache");
    bcache.heads[i].prev = &bcache.heads[i];
    bcache.heads[i].next = &bcache.heads[i];
  }

  // Create linked list of buffers

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.heads[0].next;
    b->prev = &bcache.heads[0];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[0].next->prev = b;
    bcache.heads[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint i = HASH(blockno);
  acquire(&bcache.lock[i]);

  // Is the block already cached?
  for(b = bcache.heads[i].next; b != &bcache.heads[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint minticks = ticks;
  struct buf *replace = 0;
  for(b = bcache.heads[i].prev; b != &bcache.heads[i]; b = b->prev){
    if(b->refcnt == 0 && b->timestamp <= minticks) {
      minticks = b->timestamp;
      replace = b;
    }
  }

  if(!replace)
    goto steal;

  replace->dev = dev;
  replace->blockno = blockno;
  replace->valid = 0;
  replace->refcnt = 1;
  release(&bcache.lock[i]);
  acquiresleep(&replace->lock);
  return replace;

steal:
  for(int j = 0; j < NBUCKET; j++){
    if(j == i)
      continue;
    
    acquire(&bcache.lock[j]);
    minticks = ticks;
    for(b = bcache.heads[j].prev; b != &bcache.heads[j]; b = b->prev){
      if(b->refcnt == 0 && b->timestamp <= minticks) {
        minticks = b->timestamp;
        replace = b;
      }
    }
    if(!replace){
      release(&bcache.lock[j]);
      continue;
    }

    // found! steal from another bucket(j)
    // init first
    replace->dev = dev;
    replace->blockno = blockno;
    replace->valid = 0;
    replace->refcnt = 1;

    replace->next->prev = replace->prev;
    replace->prev->next = replace->next;
    release(&bcache.lock[j]);

    replace->next = bcache.heads[i].next;
    bcache.heads[i].next->prev = replace;
    bcache.heads[i].next = replace;
    replace->prev = &bcache.heads[i];
    release(&bcache.lock[i]);
    acquiresleep(&replace->lock);
    return replace;
  }

  release(&bcache.lock[i]);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint i = HASH(b->blockno);
  acquire(&bcache.lock[i]);
  b->refcnt--;
  if (b->refcnt == 0) 
    b->timestamp = ticks;
  
  release(&bcache.lock[i]);
}

void
bpin(struct buf *b) {
  uint i = HASH(b->blockno);
  acquire(&bcache.lock[i]);
  b->refcnt++;
  release(&bcache.lock[i]);
}

void
bunpin(struct buf *b) {
  uint i = HASH(b->blockno);
  acquire(&bcache.lock[i]);
  b->refcnt--;
  release(&bcache.lock[i]);
}


