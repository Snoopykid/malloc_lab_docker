// /*
//  * mm-naive.c - The fastest, least memory-efficient malloc package.
//  *
//  * In this naive approach, a block is allocated by simply incrementing
//  * the brk pointer.  A block is pure payload. There are no headers or
//  * footers.  Blocks are never coalesced or reused. Realloc is
//  * implemented directly using mm_malloc and mm_free.
//  *
//  * NOTE TO STUDENTS: Replace this header comment with your own header
//  * comment that gives a high level description of your solution.
//  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <assert.h>
// #include <unistd.h>
// #include <string.h>

// #include "mm.h"
// #include "memlib.h"

// /*********************************************************
//  * NOTE TO STUDENTS: Before you do anything else, please
//  * provide your team information in the following struct.
//  ********************************************************/
// team_t team = {
//     /* Team name */
//     "ateam",
//     /* First member's full name */
//     "Harry Bovik",
//     /* First member's email address */
//     "bovik@cs.cmu.edu",
//     /* Second member's full name (leave blank if none) */
//     "",
//     /* Second member's email address (leave blank if none) */
//     ""};

// /* single word (4) or double word (8) alignment */
// #define ALIGNMENT 8

// /* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// // 내가 지정한 기본 상수와 매크로들
// #define WSIZE 4                                                             // 워드 와 헤더/푸터 사이즈(바이트)
// #define DSIZE 8                                                             // 더블 워드 사이즈
// #define CHUNKSIZE (1 << 12)                                                 // 이 총량의 확장 힙

// #define MAX(x, y) ((x) > (y) ? (x) : (y))

// // 블록 크기와 할당 여부 비트를 하나의 워드에 합쳐 저장
// #define PACK(size, alloc) ((size) | (alloc))

// // 주소 p에 있는 한 워드 값을 읽거나 쓴다
// #define GET(p) (*(unsigned int *)(p))
// #define PUT(p, val) (*(unsigned int *)(p) = (val))

// // 주소 p에 저장된 워드에서 블록 크기와 할당 여부를 읽는다
// #define GET_SIZE(p) (GET(p) & ~0x7)
// #define GET_ALLOC(p) (GET(p) & 0x1)

// // 블록 포인터 bp가 주어졌을 때 해당 블록의 헤더와 푸터 주소를 계산한다
// #define HDRP(bp) ((char *)(bp) - WSIZE)
// #define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// // 블록 포인터 bp가 주어졌을 때 다음 블록과 이전 블록의 주소를 계산한다
// #define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
// #define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// // 힙의 시작 기준점으로 사용할 포인터.
// // CSAPP implicit free list 구현에서는 프롤로그 블록의 payload 위치를 가리킨다.
// static char *heap_listp = NULL;


// // mm.c 내부에서만 사용하는 보조 함수들.
// // static으로 선언하면 이 파일 내부에서만 사용된다.
// static void *extend_heap(size_t words);
// static void *coalesce(void *bp);
// static void *find_fit(size_t asize);
// static void place(void *bp, size_t asize);

// /*
//  * mm_init - initialize the malloc package.
//  */
// int mm_init(void)
// {
//     // 빈 힙을 초기화하여 만드는 함수
//     if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1){return -1;}
    
//     PUT(heap_listp, 0);                             // 정렬 패딩
//     PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // 프롤로그 헤더
//     PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // 프롤로그 푸터
//     PUT(heap_listp + (3*WSIZE), PACK(0, 1));        // 에필로그 헤더
//     heap_listp += (2*WSIZE);

//     // CHUNKSIZE 바이트 크기의 가용블럭 빈 힙을 확장
//     if (extend_heap(CHUNKSIZE/WSIZE) == NULL){return -1;}

//     return 0;
// }

// static void *extend_heap(size_t words){
//     char *bp;
//     size_t size;

//     // 정렬 유지를 하는 워드들의 짝수를 할당
//     size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
//     if ((long)(bp = mem_sbrk(size)) == -1){return NULL;}

//     //  가용 블록 헤더/푸터 그리고 에필로그의 헤더를 초기화
//     PUT(HDRP(bp), PACK(size, 0));                       // 가용 블록 헤더
//     PUT(FTRP(bp), PACK(size, 0));                       // 가용 블록 푸터
//     PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));               // 새로운 에필로그 헤더

//     // 이전 블럭이 가용해지면 병합
//     return coalesce(bp);
// }

// /*
//  * mm_malloc - Allocate a block by incrementing the brk pointer.
//  *     Always allocate a block whose size is a multiple of the alignment.
//  */
// void *mm_malloc(size_t size)
// {
//     size_t asize;               // 수정된 블록 사이즈
//     size_t extendsize;          // 맞는 것이 없는 경우 확장 힙의 총량
//     char *bp;

//     // 잘못된 요청을 무시
//     if (size == 0){return NULL;}
    
//     // 오버헤드와 정렬 요청으로 블록 사이즈를 수정
//     if (size <= DSIZE){asize = 2*DSIZE;}
//     else{asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);}

//     // 딱 맞는 가용리스트를 검색
//     if ((bp = find_fit(asize)) != NULL){
//         place(bp, asize);
//         return bp;
//     }

//     // 맞는 것을 찾지 못했다면. 메모리와 블록 공간을 더 가져온다
//     extendsize = MAX(asize, CHUNKSIZE);
//     if ((bp = extend_heap(extendsize/WSIZE)) == NULL){return NULL;}

//     place(bp, asize);
//     return bp;
// }

// /*
//  * mm_free - Freeing a block does nothing.
//  */
// void mm_free(void *bp)
// {
//     size_t size = GET_SIZE(HDRP(bp));

//     PUT(HDRP(bp), PACK(size, 0));
//     PUT(FTRP(bp), PACK(size, 0));
//     coalesce(bp);
// }

// static void *find_fit(size_t asize){
//     void *bp;

//     // heap_listp부터 시작해서 다음 블록으로 이동하며 탐색
//     for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
//     {
//         if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
//         {
//             return bp;
//         }
//     }
//     // 끝까지 못 찾았으면 NULL
//     return NULL; 
// }

// static void place(void *bp, size_t asize){
//     size_t csize = GET_SIZE(HDRP(bp));

//     if ((csize - asize) >= (2 * DSIZE))
//     {
//         // 앞쪽 블록을 요청 크기만큼 allocated 상태로 만든다
//         PUT(HDRP(bp), PACK(asize, 1));
//         PUT(FTRP(bp), PACK(asize, 1));

//         // 남은 뒤쪽 블록으로 이동
//         bp = NEXT_BLKP(bp);

//         // 남은 공간을 새로운 free block으로 만든다.
//         PUT(HDRP(bp), PACK(csize - asize, 0));
//         PUT(FTRP(bp), PACK(csize - asize, 0));
//     }
//     else{
//         // 남는 공간이 최소 블록 크기보다 작으므로 현재 블록 전체를 하나의 allocated block으로 사용
//         PUT(HDRP(bp), PACK(csize, 1));
//         PUT(FTRP(bp), PACK(csize, 1));

//     }
    
// }

// static void *coalesce(void *bp){
//     size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
//     size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
//     size_t size = GET_SIZE(HDRP(bp));

//     if (prev_alloc && next_alloc){return bp;}               // case 1
//     else if (prev_alloc && !next_alloc)                     // case 2
//     {
//         size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
//         PUT(HDRP(bp), PACK(size, 0));
//         PUT(FTRP(bp), PACK(size, 0));
//     }
//     else if (!prev_alloc && next_alloc)                     // case 3
//     {
//         size += GET_SIZE(HDRP(PREV_BLKP(bp)));
//         PUT(FTRP(bp), PACK(size, 0));
//         PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
//         bp = PREV_BLKP(bp);
//     }
//     else                                                    // case 4
//     {
//         size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
//                 GET_SIZE(FTRP(NEXT_BLKP(bp)));
//         PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
//         PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
//         bp = PREV_BLKP(bp);
//     }
//     return bp;
// }

// /*
//  * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
//  */
// void *mm_realloc(void *bp, size_t size)
// {
//     void *oldptr = bp;
//     void *newptr;
//     size_t copySize;

//     newptr = mm_malloc(size);
//     if (newptr == NULL)
//         return NULL;
//     copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//     if (size < copySize)
//         copySize = size;
//     memcpy(newptr, oldptr, copySize);
//     mm_free(oldptr);
//     return newptr;
// }