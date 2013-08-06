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

#define NTV2_UNUSED_PARAMETER(p)  (void)(p)

/* ------------------------------------------------------------------------- */
/* Mutex routines                                                            */
/* ------------------------------------------------------------------------- */

static void * ntv2_mutex_create()
{
   return NTV2_NULL;
}

static void ntv2_mutex_delete(void *mp)
{
   NTV2_UNUSED_PARAMETER(mp);
}

static void ntv2_mutex_enter (void *mp)
{
   NTV2_UNUSED_PARAMETER(mp);
}

static void ntv2_mutex_leave (void *mp)
{
   NTV2_UNUSED_PARAMETER(mp);
}

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
