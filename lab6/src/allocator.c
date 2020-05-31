#include "io.h"
#include "mm.h"
#include "irq.h"
#include "util.h"
#include "string.h"
#include "allocator.h"

/* buddy system below */

Zone const buddy_zone = &(ZoneStr){
  .free_area = { [0 ... MAX_ORDER - 1] = {0, 0} }
};

void init_pages(){
#if zonealoc
  page_number = (HIGH_MEMORY - LOW_MEMORY) / (PAGE_SIZE);
  printf("need Pages[%d]" NEWLINE, page_number);
  low_memory = LOW_MEMORY + (sizeof(Page) * page_number);
  unsigned long mask = (1 << (MAX_ORDER + PAGE_SHIFT)) - 1;
  printf("mask %x" NEWLINE, mask);
  printf("new temp low %x" NEWLINE, low_memory);
  if(mask & low_memory){
    printf("rmtail low %x" NEWLINE, (~mask & low_memory));
    printf("align samll %x" NEWLINE, (1 << (MAX_ORDER + PAGE_SHIFT - 1)));
    low_memory = (~mask & low_memory) + (1 << (MAX_ORDER + PAGE_SHIFT - 1));
  }
  page_number = (HIGH_MEMORY - low_memory) / (PAGE_SIZE);
  printf("final Pages[%d]" NEWLINE, page_number);
  /* VA_START is imporant */
  low_memory |= VA_START;
  printf("new aligned low 0x%x" NEWLINE, low_memory);
  /* VA_START is imporant */
  mpages = (Page*)(LOW_MEMORY | VA_START);
  for(int i = 0; i < page_number; i++)
    mpages[i].status = empty, mpages[i].order = 0;
#endif
}

Cdr newCdr(unsigned long addr, Cdr next){
  Cdr new;
  new = (Cdr)(addr);
  new->val = addr;
  new->cdr = next;
  return new;
}

void zone_init(Zone zone){
#if !zonealoc
  return;
#endif
  init_pages();
  unsigned size = page_number, order = MAX_ORDER - 1, block;
  unsigned long addr = ALOC_BEG;
  while(order){
    block = size / (1 << order);
    Cdr *iter = &(zone->free_area[order].free_list);
    while(block){
      zone->free_area[order].nr_free++;
      *iter = newCdr(addr, 0);
      iter = &((*iter)->cdr);
      addr += (1 << (order + PAGE_SHIFT));
      block--;
    }
    size = size % (1 << order);
    order--;
  }
  buddy_log("");
  buddy_log("inited buddy graph");
  buddy_log_graph(buddy_zone);
  buddy_log("");
}

void zone_show(Zone zone, unsigned long limit){
  for(int ord = 0; ord < MAX_ORDER; ord++){
    Cdr b = zone->free_area[ord].free_list;
    unsigned long nr = zone->free_area[ord].nr_free, cnt = limit;
    printfmt("order [%d] %d items", ord, nr);
    while(b && cnt){
      printfmt("  %s── %x to %x", b->cdr ? "├" : "└",
          b->val, b->val + (1 << (ord + PAGE_SHIFT)));
      b = b->cdr;
      cnt--;
      nr--;
    }
    if(nr) printfmt("  └ %s", "...");
  }
}

unsigned long zone_get_free_pages(Zone zone, int order){
  preempt_disable();
  int ord = order;
  while(ord < MAX_ORDER &&
      !zone->free_area[ord].free_list)
    ord++;

  printfmt("nr %d list = %x", zone->free_area[ord].nr_free, zone->free_area[ord].free_list);
  if(ord == MAX_ORDER) goto buddy_allocate_failed;

  // partition to samller size
  while(ord > order){
    printfmt("ord = %d", ord);
    // 1st block
    Cdr block = zone->free_area[ord].free_list;
    zone->free_area[ord].free_list = block->cdr;
    zone->free_area[ord - 1].free_list = block;
    // 2nd block
    block->cdr = newCdr(block->val + (1 << (ord - 1 + PAGE_SHIFT)), 0);
    if(zone->free_area[ord].nr_free == 0){
      printfmt("wrong 104 0x%x", zone->free_area[ord].free_list);
      while(1);
    }
    zone->free_area[ord].nr_free -= 1;
    zone->free_area[ord - 1].nr_free += 2;
    ord--;
  }

  if(!zone->free_area[ord].free_list)
    goto buddy_allocate_failed;

  // init pages
  Cdr block = zone->free_area[ord].free_list;
  zone->free_area[ord].free_list = block->cdr;
  if(zone->free_area[ord].nr_free == 0){
    puts("wrong 119");
    while(1);
  }
  zone->free_area[ord].nr_free -= 1;
  unsigned base = addr2pgidx(block->val);

  for (int i = 0; i < (1 << order); i++){
    mpages[base + i].status = used;
    unsigned long page = ALOC_BEG + (base + i) * PAGE_SIZE;
    memzero(page, PAGE_SIZE);
  }
  mpages[base].order = order;
  buddy_log("");
  buddy_log("allocate address 0x%x", ALOC_BEG + base * PAGE_SIZE);
  buddy_log_graph(buddy_zone);
  buddy_log("");
  preempt_enable();
  return ALOC_BEG - VA_START + base * PAGE_SIZE;

buddy_allocate_failed:
  buddy_log("");
  buddy_log("buddy system allocate failed");
  buddy_log("");
  preempt_enable();
  return 0;
}

void zone_merge_buddy(Zone zone, unsigned long addr, unsigned order){
  unsigned long mask = ((1 << (order + PAGE_SHIFT + 1)) - 1);
  unsigned long buddy_addr, aligned_addr;

  if(mask & addr){
    buddy_addr = (~mask) & addr;
    aligned_addr = buddy_addr;
  }
  else{
    buddy_addr = addr + (1 << (order + PAGE_SIZE));
    aligned_addr = addr;
  }

  Cdr *iter = &(zone->free_area[order].free_list);

  while(*iter){
    if((*iter)->val == buddy_addr){
      *iter = (*iter)->cdr;
      buddy_log("buddy found, merge order %d address %x", order + 1, aligned_addr);
      zone->free_area[order].nr_free -= 1;
      return zone_merge_buddy(zone, aligned_addr, order + 1);
    }
    iter = &((*iter)->cdr);
  }

  zone->free_area[order].free_list =
    newCdr(addr, zone->free_area[order].free_list);
  zone->free_area[order].nr_free += 1;
}

void zone_free_pages(Zone zone, unsigned long addr){
  preempt_disable();
  /* important */
  addr |= VA_START;
  unsigned base = addr2pgidx(addr);
  for(int i = 0; i < (1 << mpages[base].order); i++)
    mpages[base + i].status = empty;

  buddy_log("free address 0x%x", addr);
  buddy_log("merge order %d address 0x%x", mpages[base].order, addr);
  zone_merge_buddy(zone, addr, mpages[base].order);
  buddy_log("");
  preempt_enable();
}

/* fixed size allocator below */

FixedAllocator newFixedAllocator(unsigned long addr,
    unsigned long size, FixedBook book, FixedAllocator next){
  FixedAllocator new = (FixedAllocator)addr;
  new->size = size;
  new->rs = pow2roundup(size);
  new->next = next;
  new->book = book;
  return new;
}

FixedAllocator setFixedAllocator(FixedAllocator this,
    unsigned long size){
  this->size = size;
  this->rs = pow2roundup(size);
  return this;
}

FixedBook newFixedBook(FixedBook new,
    unsigned long page_addr, FixedBook next){
  new->page_addr = page_addr;
  new->next = next;
  new->free_nr = 0;
  new->table = 0;
  return new;
}

FixedBook appendFixedBooks(){
  unsigned long addr = get_free_page();
  unsigned nr = PAGE_SIZE / sizeof(FixedBookStr);
  FixedBook iter = (FixedBook)addr;
  for(unsigned i = 1; i < nr; i++)
    iter = newFixedBook(iter, 0, (FixedBook)(addr + i * sizeof(FixedBookStr)))->next;
  newFixedBook(iter, 0, 0);
  return (FixedBook)addr;
}

unsigned long dispatch_tables(FixedBook book){
  unsigned long tables = get_free_page();
  unsigned nr = PAGE_SIZE / FixedBookTableSzie;
  for(int i = 0; i < nr && book && tables; i++, book = book->next)
    book->table = (char*)(tables + i * FixedBookTableSzie);
  return tables;
}

FixedAllocator fixed_allocator = 0;

unsigned long fixed_get_token(unsigned long size){

  disable_irq();
  if(!size) goto fixed_get_token_failed;

  FixedAllocator *iter = &fixed_allocator, fa;
  while(*iter && (*iter)->size) iter = &((*iter)->next);

  if(*iter){
    /* if exist empty fixed allocator */
    fa = *iter;
  }
  else{
    /* new allocators and books */
    unsigned long addr;
    if(!(addr = get_free_page()))
      goto fixed_get_token_failed;
    fa = (FixedAllocator)addr;
    unsigned nr = PAGE_SIZE / sizeof(FixedAllocatorStr);
    for(unsigned i = 0; i < nr; i++, iter = &((*iter)->next))
      (*iter) = newFixedAllocator(addr + i * sizeof(FixedAllocatorStr), 0, 0, 0);
    *iter = 0;
  }
  setFixedAllocator(fa, size);
  enable_irq();
  return (unsigned long)fa;

fixed_get_token_failed:
  enable_irq();
  return 0;
}

void fixed_free_token(unsigned long token){

  disable_irq();
  if(!token) goto fixed_free_token_end;

  FixedAllocator aloctor = fixed_allocator;

  while(aloctor)
    if(aloctor == (FixedAllocator)token) break;
    else aloctor = aloctor->next;

  if(!aloctor) goto fixed_free_token_end;

  aloctor->size = aloctor->rs = 0;
  FixedBook book = aloctor->book;

  while(book){
    book->free_nr = 0;
    if(book->page_addr){
      fixed_log("free page 0x%x", book->page_addr);
      free_page(book->page_addr);
    }
    if(book->table){
      if(!((unsigned long)book->table & (~PAGE_MASK))){
        fixed_log("free page 0x%x", book->table);
        free_page((unsigned long)book->table);
      }
    }
    book = book->next;
  }
  aloctor->book = 0;

fixed_free_token_end:
  enable_irq();
}

unsigned long fixed_alloc(unsigned long token){
  disable_irq();

  //show_sp();
  //__asm__ volatile("ldp	x1, x0, [sp], #96");
  //__asm__ volatile("bl show_addr");


  if(!token) goto fixed_alloc_failed;

  FixedAllocator aloctor = fixed_allocator;

  while(aloctor)
    if(aloctor == (FixedAllocator)token) break;
    else aloctor = aloctor->next;

  if(!aloctor) goto fixed_alloc_failed;

  FixedBook *bookptr = &(aloctor->book);
  while((*bookptr) && (*bookptr)->page_addr && (!(*bookptr)->free_nr))
    bookptr = &((*bookptr)->next);

  if(!(*bookptr)){
    (*bookptr) = appendFixedBooks();
  }

  if(!((*bookptr)->page_addr)){
    if(aloctor->rs > PAGE_SIZE){
      unsigned ord = 0;
      while((PAGE_SIZE << ord) < aloctor->rs) ord++;
      (*bookptr)->page_addr = zone_get_free_pages(buddy_zone, ord);
      (*bookptr)->free_nr = 1;
    }
    else{
      (*bookptr)->page_addr = get_free_page();
      (*bookptr)->free_nr = PAGE_SIZE / aloctor->rs;
    }
  }

  if(!((*bookptr)->page_addr))
    goto fixed_alloc_failed;

  if(!((*bookptr)->table) && !dispatch_tables((*bookptr)))
    goto fixed_alloc_failed;

  /* take an object from book */
  unsigned long offset = 0;
  while(btst((*bookptr)->table, offset) && offset < PAGE_SIZE){
    offset += aloctor->rs;
  }

  fixed_log("book = %d", offset);

  bset((*bookptr)->table, offset);
  (*bookptr)->free_nr -= 1;

  enable_irq();
  return (*bookptr)->page_addr + offset;

fixed_alloc_failed:
  enable_irq();
  return 0;
}

void fixed_free(unsigned long token, unsigned long addr){

  disable_irq();
  if(!token) goto fixed_free_end;

  FixedAllocator aloctor = fixed_allocator;

  while(aloctor)
    if(aloctor == (FixedAllocator)token) break;
    else aloctor = aloctor->next;

  if(!aloctor) goto fixed_free_end;

  FixedBook bookiter = aloctor->book;
  while(bookiter)
    if((bookiter->page_addr & PAGE_MASK) == (addr & PAGE_MASK))
      break;
    else bookiter = bookiter->next;

  if(!bookiter) goto fixed_free_end;

  bclr(bookiter->table, (addr - bookiter->page_addr));
  bookiter->free_nr += 1;

  /* release the page */
  if(aloctor->rs > PAGE_SIZE){
    if(bookiter->free_nr){
      fixed_log("free page 0x%x", bookiter->page_addr);
      free_page(bookiter->page_addr);
      bookiter->page_addr = 0;
    }
  }
  else{
    if(bookiter->free_nr == (PAGE_SIZE / aloctor->rs)){
      fixed_log("free page 0x%x", bookiter->page_addr);
      free_page(bookiter->page_addr);
      bookiter->page_addr = 0;
    }
  }

fixed_free_end:
  enable_irq();
}
