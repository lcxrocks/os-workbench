#include "../include/klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


void* memset(void* v,int c,size_t n) {
  int i;
  for (i = 0; i < n; i++){
    ((char*)v)[i] = (unsigned char)c;
  }
  return v;
}

void * memcpy (void * dst,const void * src,size_t count)
{
   void * ret = dst;
   if (dst <= src || (char *)dst >= ((char *)src + count))
   {  // 若dst和src区域没有重叠，则从起始处开始逐一拷贝
      while (count--)
      {
         *(char *)dst = *(char *)src;
         dst = (char *)dst + 1;
         src = (char *)src + 1;
      }
   }
   else
   {  // 若dst和src 区域交叉，则从尾部开始向起始位置拷贝，这样可以避免数据冲突
      dst = (char *)dst + count - 1;
      src = (char *)src + count - 1;
      while (count--)
      {
         *(char *)dst = *(char *)src;
         dst = (char *)dst - 1;
         src = (char *)src - 1;
      }
   }
   return(ret);
}


int memcmp(const void* s1, const void* s2, size_t n){
  int i, ans = 0;
  for (i = 0; i < n; i++){
    ans = ((char*)s1)[i] - ((char*)s2)[i];
    if (ans != 0){
      return ans;
    }
  }
  return 0;
}

void* memmove(void* dst, const void* src, size_t n)
{
    assert(NULL !=src && NULL !=dst);
    char* tmpdst = (char*)dst;
    char* tmpsrc = (char*)src;

    if (tmpdst <= tmpsrc || tmpdst >= tmpsrc + n)
    {
        while(n--)
        {
            *tmpdst++ = *tmpsrc++; 
        }
    }
    else
    {
        tmpdst = tmpdst + n - 1;
        tmpsrc = tmpsrc + n - 1;
        while(n--)
        {
            *tmpdst-- = *tmpsrc--;
        }
    }
    return dst; 
}

#endif