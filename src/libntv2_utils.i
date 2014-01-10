/* ------------------------------------------------------------------------- */
/* Copyright 2013 Esri                                                       */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License");           */
/* you may not use this file except in compliance with the License.          */
/* You may obtain a copy of the License at                                   */
/*                                                                           */
/*     http://www.apache.org/licenses/LICENSE-2.0                            */
/*                                                                           */
/* Unless required by applicable law or agreed to in writing, software       */
/* distributed under the License is distributed on an "AS IS" BASIS,         */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/* See the License for the specific language governing permissions and       */
/* limitations under the License.                                            */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* These routines are separated out so they may be replaced by the user.     */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Memory routines                                                           */
/* ------------------------------------------------------------------------- */

static void * ntv2_memalloc(size_t n)
{
   return malloc(n);
}

static void ntv2_memdealloc(void *p)
{
   if ( p != NTV2_NULL )
   {
      free(p);
   }
}

/* ------------------------------------------------------------------------- */
/* Mutex routines                                                            */
/* ------------------------------------------------------------------------- */

typedef struct ntv2_critsect NTV2_CRITSECT;

#if defined(NTV2_NO_MUTEXES)

   struct ntv2_critsect
   {
      int foo;
   };

#  define NTV2_CS_CREATE(cs)
#  define NTV2_CS_DELETE(cs)
#  define NTV2_CS_ENTER(cs)
#  define NTV2_CS_LEAVE(cs)

#elif defined(_WIN32)

#  define  WIN32_LEAN_AND_MEAN   /* Exclude rarely-used stuff */
#  include <windows.h>
   /* Note that Windows mutexes are always recursive.
   */
   struct ntv2_critsect
   {
      CRITICAL_SECTION   crit;
   };

# if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#  define NTV2_CS_CREATE(cs) \
      InitializeCriticalSectionEx (&cs.crit, 4000, 0)
# elif defined(WINCE)
#  define NTV2_CS_CREATE(cs) \
      InitializeCriticalSection   (&cs.crit)
# else
#  define NTV2_CS_CREATE(cs) \
      InitializeCriticalSectionAndSpinCount(&cs.crit, 4000)
# endif
#  define NTV2_CS_DELETE(cs) \
      DeleteCriticalSection       (&cs.crit)
#  define NTV2_CS_ENTER(cs)  \
      EnterCriticalSection        (&cs.crit)
#  define NTV2_CS_LEAVE(cs)  \
      LeaveCriticalSection        (&cs.crit)

#else

#  include <pthread.h>
   /* In Unix we always use POSIX threads & mutexes.
      We also make sure that our mutexes are recursive
      (by default, POSIX mutexes are not recursive).
   */
   struct ntv2_critsect
   {
      pthread_mutex_t       crit;
      pthread_mutexattr_t   attr;
   };

#  if linux
#    define MUTEX_RECURSIVE   PTHREAD_MUTEX_RECURSIVE_NP
#  else
#    define MUTEX_RECURSIVE   PTHREAD_MUTEX_RECURSIVE
#  endif

#  define NTV2_CS_CREATE(cs)   \
      pthread_mutexattr_init    (&cs.attr);                  \
      pthread_mutexattr_settype (&cs.attr, MUTEX_RECURSIVE); \
      pthread_mutex_init        (&cs.crit, &cs.attr)

#  define NTV2_CS_DELETE(cs)   \
      pthread_mutex_destroy     (&cs.crit); \
      pthread_mutexattr_destroy (&cs.attr)

#  define NTV2_CS_ENTER(cs)   \
      pthread_mutex_lock        (&cs.crit)

#  define NTV2_CS_LEAVE(cs)   \
      pthread_mutex_unlock      (&cs.crit)

#endif /* OS-specific stuff */

typedef struct ntv2_mutex_t NTV2_MUTEX_T;
struct ntv2_mutex_t
{
   NTV2_CRITSECT  cs;
   int            count;
};

static void * ntv2_mutex_create()
{
   NTV2_MUTEX_T * m = (NTV2_MUTEX_T *)ntv2_memalloc(sizeof(*m));

   if ( m != NTV2_NULL )
   {
      m->count = 0;
      NTV2_CS_CREATE(m->cs);
   }

   return (void *)m;
}

static void ntv2_mutex_enter (void *mp)
{
   NTV2_MUTEX_T * m = (NTV2_MUTEX_T *)mp;

   if ( m != NTV2_NULL )
   {
      NTV2_CS_ENTER(m->cs);
      m->count++;
   }
}

static void ntv2_mutex_leave (void *mp)
{
   NTV2_MUTEX_T * m = (NTV2_MUTEX_T *)mp;

   if ( m != NTV2_NULL )
   {
      m->count--;
      NTV2_CS_LEAVE(m->cs);
   }
}

static void ntv2_mutex_delete(void *mp)
{
   NTV2_MUTEX_T * m = (NTV2_MUTEX_T *)mp;

   if ( m != NTV2_NULL )
   {
      while (m->count > 0)
      {
         ntv2_mutex_leave(m);
      }
      NTV2_CS_DELETE(m->cs);

      ntv2_memdealloc(m);
   }
}
