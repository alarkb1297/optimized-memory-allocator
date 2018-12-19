Operating System/Cores: CentOS 7.3.1611 - 8 Processor Cores

My strategy for implementing a faster allocator included using
a version of a binning strategy. My bins were all thread local 
variables so each thread had its own bins. The bins were split up by
increasing powers of 2 starting from 32. So the bins were 32, 64, 128,
256, 512, 1024, and 2048. When allocating memory, I would always
round up the required size to the nearest power of 2, so retrieving  
from correct bins, and allocating to the bin would be standardized.
The bins pull memory from the thread local block of memory. This block
 maintains memory in sizes of full pages. If the block is empty, 
it will mmap 50 pages, filling the global block. If a certain bin has no 
memory left, it will checkout a full page from the global block (or
number of pages required if size required is greater than a page), then
returns what the user needs and puts the rest to the appropriate bin.
This limits the number of mmaps called, speeding up the allocator. 
