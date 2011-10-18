/*==================================================================*/
/* xmp_io.c                                                         */
/* Copyright (C) 2011, FUJITSU LIMITED                              */
/*==================================================================*\
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation;
  version 2.1 published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
\*==================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xmp_constant.h"
#include "xmp_data_struct.h"
#include "xmp_io.h"

//#define DEBUG

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fopen_all                                            */
/*  DESCRIPTION   : ���δؿ���XcalableMP�ѤΥե�����򳫤���                 */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : fname[IN] �ե�����̾                                     */
/*                  amode[IN] POSIX��fopen������"rwa+"���ȹ礻��             */
/*  RETURN VALUES : xmp_file_t* �ե����빽¤�Ρ��۾ｪλ�ξ���NULL���֤��� */
/*                                                                           */
/*****************************************************************************/
xmp_file_t *xmp_fopen_all(const char *fname, const char *amode)
{
  xmp_file_t *pstXmp_file = NULL;
  int         iMode = 0;
  size_t      modelen = 0;


  // �ΰ����
  pstXmp_file = malloc(sizeof(xmp_file_t));
  if (pstXmp_file == NULL) { return NULL; } 
  memset(pstXmp_file, 0x00, sizeof(xmp_file_t));
  
  ///
  /// �⡼�ɲ���
  ///
  modelen = strlen(amode);
  // �⡼�ɤ���ʸ��
  if (modelen == 1)
  {
    if (strncmp(amode, "r", modelen) == 0)
    {
      iMode = MPI_MODE_RDONLY;
    }
    else if (strncmp(amode, "w", modelen) == 0)
    {
      iMode = (MPI_MODE_WRONLY | MPI_MODE_CREATE);
    }
    else if (strncmp(amode, "a", modelen) == 0)
    {
      iMode = (MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_APPEND);
      pstXmp_file->is_append = 0x01;
    }
    else
    {
      goto ErrorExit;
    }
  }
  // �⡼�ɣ�ʸ��
  else if (modelen == 2)
  {
    if (strncmp(amode, "r+", modelen) == 0)
    {
      iMode = MPI_MODE_RDWR;
    }
    else if (strncmp(amode, "w+", modelen) == 0)
    {
      iMode = (MPI_MODE_RDWR | MPI_MODE_CREATE);
    }
    else if (strncmp(amode, "a+", modelen) == 0 ||
             strncmp(amode, "ra", modelen) == 0 ||
             strncmp(amode, "ar", modelen) == 0)
    {
      iMode = (MPI_MODE_RDWR | MPI_MODE_CREATE);
      pstXmp_file->is_append = 0x01;
    }
    else if (strncmp(amode, "rw", modelen) == 0 ||
             strncmp(amode, "wr", modelen) == 0)
    {
        goto ErrorExit;
    }
    else
    {
      goto ErrorExit;
    }
  }
  // �⡼�ɤ���¾
  else
  {
    goto ErrorExit;
  }

  // �ե����륪���ץ�
  if (MPI_File_open(MPI_COMM_WORLD,
                    (char*)fname,
                    iMode,
                    MPI_INFO_NULL,
                    &(pstXmp_file->fh)) != MPI_SUCCESS)
  {
    goto ErrorExit;
  }

  // "W" or "W+"�ξ��ϥե����륵�����򣰤ˤ���
  if ((iMode == (MPI_MODE_WRONLY | MPI_MODE_CREATE)  ||
       iMode == (MPI_MODE_RDWR   | MPI_MODE_CREATE)) &&
       pstXmp_file->is_append == 0x00)
  {
    if (MPI_File_set_size(pstXmp_file->fh, 0) != MPI_SUCCESS)
    {
      goto ErrorExit;
    }
  }

  // ���ｪλ
  return pstXmp_file;

// ���顼��
ErrorExit:
  if (pstXmp_file != NULL)
  {
    free(pstXmp_file);
  }
  return NULL;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fclose_all                                           */
/*  DESCRIPTION   : ���δؿ���XcalableMP�ѤΥե�������Ĥ��롣               */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*  RETURN VALUES : ���ｪλ�ξ���0���֤���                                */
/*                  �۾ｪλ�ξ���0�ʳ����ͤ��֤���                        */
/*                                                                           */
/*****************************************************************************/
int xmp_fclose_all(xmp_file_t *pstXmp_file)
{
  // ���������å�
  if (pstXmp_file == NULL)     { return 1; }

  // �ե����륯����
  if (MPI_File_close(&(pstXmp_file->fh)) != MPI_SUCCESS)
  {
    free(pstXmp_file);
    return 1;
  }

  free(pstXmp_file);
  return 0;
}


/*****************************************************************************/
/*  FUNCTION NAME : xmp_fseek                                                */
/*  DESCRIPTION   : ���δؿ��ϡ�fh�θ�ͭ�ե�����ݥ��󥿤ΰ��֤��ѹ����롣   */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  offset[IN] whence�ΰ��֤��鸽�ߤΥե�����ӥ塼���Ѱ�    */
/*                  whence[IN] �ե�����ΰ��֤�����                          */
/*  RETURN VALUES : ���ｪλ�ξ���0���֤���                                */
/*                  �۾ｪλ�ξ���0�ʳ����ͤ��֤���                        */
/*                                                                           */
/*****************************************************************************/
int xmp_fseek(xmp_file_t *pstXmp_file, long long offset, int whence)
{
  int iMpiWhence;

  // ���������å�
  if (pstXmp_file == NULL) { return 1; }

  // offset��MPI_Offset���Ѵ�
  switch (whence)
  {
    case SEEK_SET:
      iMpiWhence = MPI_SEEK_SET;
      break;
    case SEEK_CUR:
      iMpiWhence = MPI_SEEK_CUR;
      break;
    case SEEK_END:
      iMpiWhence = MPI_SEEK_END;
      break;
    default:
      return 1;
  }

  // �ե����륷����
  if (MPI_File_seek(pstXmp_file->fh, (MPI_Offset)offset, iMpiWhence) != MPI_SUCCESS)
  {
    return 1;
  }

  return 0;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fseek_shared_all                                     */
/*  DESCRIPTION   : ���δؿ��ϡ�fh�ζ�ͭ�ե�����ݥ��󥿤ΰ��֤��ѹ����롣   */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  offset[IN] whence�ΰ��֤��鸽�ߤΥե�����ӥ塼���Ѱ�    */
/*                  whence[IN] �ե�����ΰ��֤�����                          */
/*  RETURN VALUES : ���ｪλ�ξ���0���֤���                                */
/*                  �۾ｪλ�ξ���0�ʳ����ͤ��֤���                        */
/*                                                                           */
/*****************************************************************************/
int xmp_fseek_shared(xmp_file_t *pstXmp_file, long long offset, int whence)
{
  int iMpiWhence;

  // ���������å�
  if (pstXmp_file == NULL) { return 1; }

  // offset��MPI_Offset���Ѵ�
  switch (whence)
  {
    case SEEK_SET:
      iMpiWhence = MPI_SEEK_SET;
      break;
    case SEEK_CUR:
      iMpiWhence = MPI_SEEK_CUR;
      break;
    case SEEK_END:
      iMpiWhence = MPI_SEEK_END;
      break;
    default:
      return 1;
  }

  // �ե����륷����
  if (MPI_File_seek_shared(pstXmp_file->fh, (MPI_Offset)offset, iMpiWhence) != MPI_SUCCESS)
  {
    return 1;
  }

  return 0;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_ftell                                                */
/*  DESCRIPTION   : ���δؿ��ϡ�fh�θ�ͭ�ե�����ݥ��󥿤Υե�������Ƭ����� */
/*                  �Ѱ̤���롣                                           */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*  RETURN VALUES : ���ｪλ�ξ��ϥե�������Ƭ������Ѱ�(byte)���֤���     */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
long long xmp_ftell(xmp_file_t *pstXmp_file)
{
  MPI_Offset offset;
  MPI_Offset disp;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }

  if (MPI_File_get_position(pstXmp_file->fh, &offset) != MPI_SUCCESS)
  {
    return -1;
  }

  if (MPI_File_get_byte_offset(pstXmp_file->fh, offset, &disp) != MPI_SUCCESS)
  {
    return -1;
  }

  return (long long)disp;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_ftell_shared                                         */
/*  DESCRIPTION   : ���δؿ��ϡ�fh�ζ�ͭ�ե�����ݥ��󥿤Υե�������Ƭ����� */
/*                  �Ѱ̤���롣                                           */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*  RETURN VALUES : ���ｪλ�ξ��ϥե�������Ƭ������Ѱ�(byte)���֤���     */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
long long xmp_ftell_shared(xmp_file_t *pstXmp_file)
{
  MPI_Offset offset;
  MPI_Offset disp;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }

  if (MPI_File_get_position_shared(pstXmp_file->fh, &offset) != MPI_SUCCESS)
  {
    return -1;
  }

  if (MPI_File_get_byte_offset(pstXmp_file->fh, offset, &disp) != MPI_SUCCESS)
  {
    return -1;
  }

  return (long long)disp;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_file_sync_all                                        */
/*  DESCRIPTION   : ���δؿ��ϡ��ե������Ʊ����Ԥ���                       */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*  RETURN VALUES : ���ｪλ�ξ���0���֤���                                */
/*                  �۾ｪλ�ξ���0�ʳ����ͤ��֤���                        */
/*                                                                           */
/*****************************************************************************/
long long xmp_file_sync_all(xmp_file_t *pstXmp_file)
{
  // ���������å�
  if (pstXmp_file == NULL) { return 1; }

  // Ʊ�� 
  if (MPI_File_sync(pstXmp_file->fh) != MPI_SUCCESS)
  {
    return 1;
  }

  // �Хꥢ
  MPI_Barrier(MPI_COMM_WORLD);

  return 0;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fread_all                                            */
/*  DESCRIPTION   : ���δؿ��ϼ¹Ԥ����Ρ��ɤ�buffer�إե�����ӥ塼�˽���   */
/*                  �ǡ������ɹ��ࡣ                                         */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  buffer[OUT] �ǡ������ɹ����ѿ�����Ƭ���ɥ쥹             */
/*                  size[IN]  �ɹ���ǡ�����1��������Υ����� (�Х���)       */
/*                  count[IN] �ɹ���ǡ����ο�                               */
/*  RETURN VALUES : ���ｪλ�ξ����ɹ�����Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fread_all(xmp_file_t *pstXmp_file, void *buffer, size_t size, size_t count)
{
  MPI_Status status;
  int readCount;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (buffer      == NULL) { return -1; }
  if (size  < 1) { return -1; }
  if (count < 1) { return -1; }

  // �ɹ���
  if (MPI_File_read_all(pstXmp_file->fh,
                        buffer, size * count,
                        MPI_BYTE,
                        &status) != MPI_SUCCESS)
  {
    return -1;
  }
  
  // �ɹ�����Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &readCount) != MPI_SUCCESS)
  {
    return -1;
  }

  return readCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fwrite_all                                           */
/*  DESCRIPTION   : ���δؿ��ϼ¹Ԥ����Ρ��ɤ�buffer����ե�����ӥ塼��     */
/*                  �����ǡ��������ࡣ                                     */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  buffer[IN] �ǡ����������ѿ�����Ƭ���ɥ쥹              */
/*                  size[IN] �����ǡ�����1��������Υ����� (�Х���)        */
/*                  count[IN] �����ǡ����ο�                               */
/*  RETURN VALUES : ���ｪλ�ξ��Ͻ������Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fwrite_all(xmp_file_t *pstXmp_file, void *buffer, size_t size, size_t count)
{
  MPI_Status status;
  int writeCount;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (buffer      == NULL) { return -1; }
  if (size  < 1) { return -1; }
  if (count < 1) { return -1; }

  // �ե����륪���ץ�"r+"�ξ��Ͻ�ü�˥ݥ��󥿤��ư
  if (pstXmp_file->is_append)
  {
    if (MPI_File_seek(pstXmp_file->fh,
                      (MPI_Offset)0,
                      MPI_SEEK_END) != MPI_SUCCESS)
    {
      return -1;
    }

    pstXmp_file->is_append = 0x00;
  }

  // �����
  if (MPI_File_write_all(pstXmp_file->fh, buffer, size * count, MPI_BYTE, &status) != MPI_SUCCESS)
  {
    return -1;
  }

  // �������Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &writeCount) != MPI_SUCCESS)
  {
    return -1;
  }

  return writeCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fread_darray_unpack                                  */
/*  DESCRIPTION   : ���δؿ���ap�ǻ��ꤵ���ʬ������ˤĤ��ơ�rp�ǻ��ꤵ��� */
/*                  �ϰϤإե����뤫��ǡ������ɹ��ࡣ                       */
/*                  �����������������˥���ѥå����롣                       */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : fp[IN] �ե����빽¤��                                    */
/*                  ap[IN/OUT] ʬ���������                                  */
/*                  rp[IN]     ���������ϰϾ���                              */
/*  RETURN VALUES : ���ｪλ�ξ����ɹ�����Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
int xmp_fread_darray_unpack(fp, ap, rp)
     xmp_file_t *fp;
     xmp_array_t ap;
     xmp_range_t *rp;
{
   MPI_Status    status;
   _XMP_array_t *array_t;
   char         *array_addr;
   char         *buf=NULL;
   char         *cp;
   int          *lb=NULL;
   int          *ub=NULL;
   int          *step=NULL;
   int          *cnt=NULL;
   int           buf_size;
   int           ret=0;
   int           disp;
   int           size;
   int           array_size;
   int           i, j;

   array_t = (_XMP_array_t*)ap;
  
   /* ��ž���򼨤�������� */
   lb = (int*)malloc(sizeof(int)*rp->dims);
   ub = (int*)malloc(sizeof(int)*rp->dims);
   step = (int*)malloc(sizeof(int)*rp->dims);
   cnt = (int*)malloc(sizeof(int)*rp->dims);
   if(!lb || !ub || !step || !cnt){
      ret = -1;
      goto FunctionExit;
   }
  
   /* ��ž������� */
   buf_size = 1;
   for(i=0; i<rp->dims; i++){
      /* error check */
      if(rp->step[i] > 0 && rp->lb[i] > rp->ub[i]){
         ret = -1;
         goto FunctionExit;
      }
      if(rp->step[i] < 0 && rp->lb[i] < rp->ub[i]){
         ret = -1;
         goto FunctionExit;
      }
      if (array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
          array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION) {
         lb[i] = rp->lb[i];
         ub[i] = rp->ub[i];
         step[i] = rp->step[i];
  
      } else if(array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK){
         if(rp->step[i] > 0){
            if(array_t->info[i].par_upper < rp->lb[i] ||
               array_t->info[i].par_lower > rp->ub[i]){
               lb[i] = 1;
               ub[i] = 0;
               step[i] = 1;
            } else {
               lb[i] = (array_t->info[i].par_lower > rp->lb[i])?
                  rp->lb[i]+((array_t->info[i].par_lower-1-rp->lb[i])/rp->step[i]+1)*rp->step[i]:
                  rp->lb[i];
               ub[i] = (array_t->info[i].par_upper < rp->ub[i]) ?
                  array_t->info[i].par_upper:
                  rp->ub[i];
               step[i] = rp->step[i];
            }
         } else {
            if(array_t->info[i].par_upper < rp->ub[i] ||
               array_t->info[i].par_lower > rp->lb[i]){
               lb[i] = 1;
               ub[i] = 0;
               step[i] = 1;
            } else {
               lb[i] = (array_t->info[i].par_upper < rp->lb[i])?
                  rp->lb[i]-((rp->lb[i]-array_t->info[i].par_upper-1)/rp->step[i]-1)*rp->step[i]:
                  rp->lb[i];
               ub[i] = (array_t->info[i].par_lower > rp->ub[i])?
                  array_t->info[i].par_lower:
                  rp->ub[i];
               step[i] = rp->step[i];
            }
         }
      } else {
         ret = -1;
         goto FunctionExit;
      }
      cnt[i] = (ub[i]-lb[i]+step[i])/step[i];
      cnt[i] = (cnt[i]>0)? cnt[i]: 0;
      buf_size *= cnt[i];
   }
  
   /* �Хåե����� */
   if(buf_size == 0){
      buf = (char*)malloc(array_t->type_size);
   } else {
      buf = (char*)malloc(buf_size*array_t->type_size);
   }
   if(!buf){
      ret = -1;
      goto FunctionExit;
   }

   // �����
   if(buf_size > 0){
      if (MPI_File_read(fp->fh, buf, buf_size*array_t->type_size, MPI_BYTE, &status) != MPI_SUCCESS) {
         ret = -1;
         goto FunctionExit;
      }
      
      // �ɤ߹�����Х��ȿ�
      if (MPI_Get_count(&status, MPI_BYTE, &ret) != MPI_SUCCESS) {
         ret = -1;
         goto FunctionExit;
      }
   } else {
      ret = 0;
   }
  
   /* �ǡ����򥢥�ѥå� */
   cp = buf;
   array_addr = (char*)(*array_t->array_addr_p);
   for(j=0; j<buf_size; j++){
      disp = 0;
      size = 1;
      array_size = 1;
      for(i=rp->dims-1; i>=0; i--){
         ub[i] = (j/size)%cnt[i];
         if (array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
             array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION) {
            disp += (lb[i]+ub[i]*step[i])*array_size;
            array_size *= array_t->info[i].ser_size;
         } else if(array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK){
            disp += (lb[i]+ub[i]*step[i]+array_t->info[i].local_lower-array_t->info[i].par_lower)*array_size;
            array_size *= array_t->info[i].alloc_size;
         }
         size *= cnt[i];
      }
      disp *= array_t->type_size;
      memcpy(array_addr+disp, cp, array_t->type_size);
      cp += array_t->type_size;
   }

 FunctionExit:
   if(buf) free(buf);
   if(lb) free(lb);
   if(ub) free(ub);
   if(step) free(step);
   if(cnt) free(cnt);

   return ret;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fread_darray_all                                     */
/*  DESCRIPTION   : ���δؿ���ap�ǻ��ꤵ���ʬ������ˤĤ��ơ�rp�ǻ��ꤵ��� */
/*                  �ϰϤإե����뤫��ǡ������ɹ��ࡣ                       */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  ap[IN/OUT] ʬ���������                                  */
/*                  rp[IN]     ���������ϰϾ���                              */
/*  RETURN VALUES : ���ｪλ�ξ����ɹ�����Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fread_darray_all(xmp_file_t  *pstXmp_file,
                            xmp_array_t  ap,
                            xmp_range_t *rp)
{
  _XMP_array_t *XMP_array_t;
  MPI_Status status;        // MPI���ơ�����
  int readCount;            // �꡼�ɥХ���
  int mpiRet;               // MPI�ؿ������
  int lower;                // ���ΥΡ��ɤǥ����������벼��
  int upper;                // ���ΥΡ��ɤǥ�������������
  int continuous_size;      // Ϣ³�襵����
  int space_size;           // ��֥�����
  int total_size;           // ���Υ�����
  int type_size;
  MPI_Aint tmp1, tmp2;
  MPI_Datatype dataType[2];
  int i = 0;
#ifdef DEBUG
  int rank, nproc;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (ap == NULL)          { return -1; }
  if (rp == NULL)          { return -1; }

  /* ����ѥå���ɬ�פʾ����̽��� */
  for (i = rp->dims - 1; i >= 0; i--){
     if(rp->step[i] < 0){
        int ret = xmp_fread_darray_unpack(pstXmp_file, ap, rp);
        return ret;
     }
  }

  XMP_array_t = (_XMP_array_t*)ap; 

  // �������Υ����å�
  if (XMP_array_t->dim != rp->dims) { return -1; }
#ifdef DEBUG
printf("READ(%d/%d) dims=%d\n", rank, nproc, rp->dims);
#endif

  // ���ܥǡ������κ���
  MPI_Type_contiguous(XMP_array_t->type_size, MPI_BYTE, &dataType[0]);

  // ������ʬ�롼��
  for (i = rp->dims - 1; i >= 0; i--)
  {
#ifdef DEBUG
printf("READ(%d/%d) (lb,ub,step)=(%d,%d,%d)\n",
       rank, nproc, rp->lb[i],  rp->ub[i], rp->step[i]);
printf("READ(%d/%d) (par_lower,par_upper)=(%d,%d)\n",
       rank, nproc, XMP_array_t->info[i].par_lower, XMP_array_t->info[i].par_upper);
#endif
    // ʬ����̵������
    if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
        XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION)
    {
      // ʬ����� < ����
      if (XMP_array_t->info[i].par_upper < rp->lb[i]) { return -1; }
      // ʬ��岼�� > ���
      if (XMP_array_t->info[i].par_lower > rp->ub[i]) { return -1; }

      // ��ʬ����
      if ( rp->step[i] < 0)
      {
      }
      // ��ʬ����
      else
      {
        // Ϣ³��Υ�����
        continuous_size = (rp->ub[i] - rp->lb[i]) / rp->step[i] + 1;

        // �ǡ��������ϰϤ����
        mpiRet =MPI_Type_get_extent(dataType[0], &tmp1, &tmp2);
        if (mpiRet !=  MPI_SUCCESS) { return -1; }  
        type_size = (int)tmp2;

        // ���ܥǡ�����������
        mpiRet = MPI_Type_create_hvector(continuous_size,
                                         1,
                                         type_size * rp->step[i],
                                         dataType[0],
                                         &dataType[1]);

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[0]);

        // MPI_Type_create_hvector�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ���Υ�����
        total_size
          = (XMP_array_t->info[i].par_upper
          -  XMP_array_t->info[i].par_lower + 1)
          *  type_size;

        // ��֥�����
        space_size
          = (rp->lb[i] - XMP_array_t->info[i].par_lower)
          * type_size;

        // �������ե����뷿�κ���
        mpiRet = MPI_Type_create_resized(dataType[1],
                                         (MPI_Aint)space_size,
                                         (MPI_Aint)total_size,
                                         &dataType[0]);

        // MPI_Type_create_resized�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[1]);

#ifdef DEBUG
printf("READ(%d/%d) NOT_ALIGNED\n", rank, nproc);
printf("READ(%d/%d) continuous_size=%d\n", rank, nproc, continuous_size);
printf("READ(%d/%d) space_size=%d\n", rank, nproc, space_size);
printf("READ(%d/%d) total_size=%d\n", rank, nproc, total_size);
#endif
      }
    }
     // blockʬ��
    else if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK)
    {
      // ��ʬ����
      if ( rp->step[i] < 0)
      {
      }
      // ��ʬ����
      else
      {
        // ʬ����� < ����
        if (XMP_array_t->info[i].par_upper < rp->lb[i])
        {
          continuous_size = 0;
        }
        // ʬ��岼�� > ���
        else if (XMP_array_t->info[i].par_lower > rp->ub[i])
        {
          continuous_size = 0;
        }
        // ����¾
        else
        {
          // �Ρ��ɤβ���
          lower
            = (XMP_array_t->info[i].par_lower > rp->lb[i]) ?
              rp->lb[i] + ((XMP_array_t->info[i].par_lower - 1 - rp->lb[i]) / rp->step[i] + 1) * rp->step[i]
            : rp->lb[i];

          // �Ρ��ɤξ��
          upper
            = (XMP_array_t->info[i].par_upper < rp->ub[i]) ?
               XMP_array_t->info[i].par_upper : rp->ub[i];

          // Ϣ³���ǿ�
          continuous_size = (upper - lower + rp->step[i]) / rp->step[i];
        }

        // �ǡ��������ϰϤ����
        mpiRet =MPI_Type_get_extent(dataType[0], &tmp1, &tmp2);
        if (mpiRet !=  MPI_SUCCESS) { return -1; }  
        type_size = (int)tmp2;

        // ���ܥǡ�����������
        mpiRet = MPI_Type_create_hvector(continuous_size,
                                         1,
                                         type_size * rp->step[i],
                                         dataType[0],
                                         &dataType[1]);

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[0]);

        // MPI_Type_create_hvector�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ��֥�����
        space_size
          = (XMP_array_t->info[i].local_lower
          + (lower - XMP_array_t->info[i].par_lower))
          * type_size;

        // ���Υ�����
        total_size = (XMP_array_t->info[i].alloc_size)* type_size;

        // �������ե����뷿�κ���
        mpiRet = MPI_Type_create_resized(dataType[1],
                                         (MPI_Aint)space_size,
                                         (MPI_Aint)total_size,
                                         &dataType[0]);

        // MPI_Type_create_resized�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[1]);

#ifdef DEBUG
printf("READ(%d/%d) ALIGN_BLOCK\n", rank, nproc);
printf("READ(%d/%d) continuous_size=%d\n", rank, nproc, continuous_size);
printf("READ(%d/%d) space_size=%d\n", rank, nproc, space_size);
printf("READ(%d/%d) total_size=%d\n", rank, nproc, total_size);
printf("READ(%d/%d) (lower,upper)=(%d,%d)\n", rank, nproc, lower, upper);
#endif
      }
    }
    // cyclicʬ��
    else if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_CYCLIC)
    {
      return -1;
    }
    // ����¾
    else
    {
      return -1;
    }
  }

  // ���ߥå�
  mpiRet = MPI_Type_commit(&dataType[0]);

  // ���ߥåȤ����顼�ξ��
  if (mpiRet != MPI_SUCCESS) { return 1; }
  
  // �ɹ���
  MPI_Type_size(dataType[0], &type_size);
  if(type_size > 0){
     if (MPI_File_read(pstXmp_file->fh,
                       (*XMP_array_t->array_addr_p),
                       1,
                       dataType[0],
                       &status)
         != MPI_SUCCESS)
        {
           return -1;
        }
  } else {
     return 0;
  }
  
  // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
  MPI_Type_free(&dataType[0]);

  // �ɹ�����Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &readCount) != MPI_SUCCESS)
  {
    return -1;
  }
  return readCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fwrite_darray_pack                                   */
/*  DESCRIPTION   : ���δؿ���ap�ǻ��ꤵ���ʬ������ˤĤ��ơ�rp�ǻ��ꤵ��� */
/*                  �ϰϤ���ե�����إǡ��������ࡣ                       */
/*                  �����������������˥ѥå����롣                           */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : fp[IN] �ե����빽¤��                                    */
/*                  ap[IN] ʬ���������                                      */
/*                  rp[IN]     ���������ϰϾ���                              */
/*  RETURN VALUES : ���ｪλ�ξ��Ͻ������Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
int xmp_fwrite_darray_pack(fp, ap, rp)
     xmp_file_t *fp;
     xmp_array_t ap;
     xmp_range_t *rp;
{
   MPI_Status    status;
   _XMP_array_t *array_t;
   char         *array_addr;
   char         *buf=NULL;
   char         *cp;
   int          *lb=NULL;
   int          *ub=NULL;
   int          *step=NULL;
   int          *cnt=NULL;
   int           buf_size;
   int           ret=0;
   int           disp;
   int           size;
   int           array_size;
   int           i, j;

   array_t = (_XMP_array_t*)ap;
  
   /* ��ž���򼨤�������� */
   lb = (int*)malloc(sizeof(int)*rp->dims);
   ub = (int*)malloc(sizeof(int)*rp->dims);
   step = (int*)malloc(sizeof(int)*rp->dims);
   cnt = (int*)malloc(sizeof(int)*rp->dims);
   if(!lb || !ub || !step || !cnt){
      ret = -1;
      goto FunctionExit;
   }
  
   /* ��ž������� */
   buf_size = 1;
   for(i=0; i<rp->dims; i++){
      /* error check */
      if(rp->step[i] > 0 && rp->lb[i] > rp->ub[i]){
         ret = -1;
         goto FunctionExit;
      }
      if(rp->step[i] < 0 && rp->lb[i] < rp->ub[i]){
         ret = -1;
         goto FunctionExit;
      }
      if (array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
          array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION) {
         lb[i] = rp->lb[i];
         ub[i] = rp->ub[i];
         step[i] = rp->step[i];
  
      } else if(array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK){
         if(rp->step[i] > 0){
            if(array_t->info[i].par_upper < rp->lb[i] ||
               array_t->info[i].par_lower > rp->ub[i]){
               lb[i] = 1;
               ub[i] = 0;
               step[i] = 1;
            } else {
               lb[i] = (array_t->info[i].par_lower > rp->lb[i])?
                  rp->lb[i]+((array_t->info[i].par_lower-1-rp->lb[i])/rp->step[i]+1)*rp->step[i]:
                  rp->lb[i];
               ub[i] = (array_t->info[i].par_upper < rp->ub[i]) ?
                  array_t->info[i].par_upper:
                  rp->ub[i];
               step[i] = rp->step[i];
            }
         } else {
            if(array_t->info[i].par_upper < rp->ub[i] ||
               array_t->info[i].par_lower > rp->lb[i]){
               lb[i] = 1;
               ub[i] = 0;
               step[i] = 1;
            } else {
               lb[i] = (array_t->info[i].par_upper < rp->lb[i])?
                  rp->lb[i]-((rp->lb[i]-array_t->info[i].par_upper-1)/rp->step[i]-1)*rp->step[i]:
                  rp->lb[i];
               ub[i] = (array_t->info[i].par_lower > rp->ub[i])?
                  array_t->info[i].par_lower:
                  rp->ub[i];
               step[i] = rp->step[i];
            }
         }
      } else {
         ret = -1;
         goto FunctionExit;
      }
      cnt[i] = (ub[i]-lb[i]+step[i])/step[i];
      cnt[i] = (cnt[i]>0)? cnt[i]: 0;
      buf_size *= cnt[i];

#ifdef DEBUG
      fprintf(stderr, "dim = %d: (%d: %d: %d) %d\n", i, lb[i], ub[i], step[i], buf_size);
#endif
   }
  
   /* �Хåե����� */
   if(buf_size == 0){
      buf = (char*)malloc(array_t->type_size);
      fprintf(stderr, "size = 0\n");
   } else {
      buf = (char*)malloc(buf_size*array_t->type_size);
   }
   if(!buf){
      ret = -1;
      goto FunctionExit;
   }

   /* �ǡ�����ѥå� */
   cp = buf;
   array_addr = (char*)(*array_t->array_addr_p);
   for(j=0; j<buf_size; j++){
      disp = 0;
      size = 1;
      array_size = 1;
      for(i=rp->dims-1; i>=0; i--){
         ub[i] = (j/size)%cnt[i];
         if (array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
             array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION) {
            disp += (lb[i]+ub[i]*step[i])*array_size;
            array_size *= array_t->info[i].ser_size;
         } else if(array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK){
            disp += (lb[i]+ub[i]*step[i]+array_t->info[i].local_lower-array_t->info[i].par_lower)*array_size;
            array_size *= array_t->info[i].alloc_size;
         }
         size *= cnt[i];
      }
      disp *= array_t->type_size;
      memcpy(cp, array_addr+disp, array_t->type_size);
      cp += array_t->type_size;
   }

  // �����
   if(buf_size > 0){
      if (MPI_File_write(fp->fh, buf, buf_size*array_t->type_size, MPI_BYTE, &status) != MPI_SUCCESS) {
         ret = -1;
         goto FunctionExit;
      }
      
      // �񤭹�����Х��ȿ�
      if (MPI_Get_count(&status, MPI_BYTE, &ret) != MPI_SUCCESS) {
         ret = -1;
         goto FunctionExit;
      }
   } else {
      ret = 0;
   }
  
 FunctionExit:
   if(buf) free(buf);
   if(lb) free(lb);
   if(ub) free(ub);
   if(step) free(step);
   if(cnt) free(cnt);

   return ret;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fwrite_darray_all                                    */
/*  DESCRIPTION   : ���δؿ���ap�ǻ��ꤵ���ʬ������ˤĤ��ơ�rp�ǻ��ꤵ��� */
/*                  �ϰϤΥǡ�����ե�����˽���ࡣ                         */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  ap[IN/OUT] ʬ���������                                  */
/*                  rp[IN]     ���������ϰϾ���                              */
/*  RETURN VALUES : ���ｪλ�ξ����ɹ�����Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fwrite_darray_all(xmp_file_t *pstXmp_file,
                             xmp_array_t ap,
                             xmp_range_t *rp)
{
  _XMP_array_t *XMP_array_t;
  MPI_Status status;        // MPI���ơ�����
  int writeCount;           // �饤�ȥХ���
  int mpiRet;               // MPI�ؿ������
  int lower;                // ���ΥΡ��ɤǥ����������벼��
  int upper;                // ���ΥΡ��ɤǥ�������������
  int continuous_size;      // Ϣ³�襵����
  int space_size;           // ��֥�����
  int total_size;           // ���Υ�����
  int type_size;
  MPI_Aint tmp1, tmp2;
  MPI_Datatype dataType[2];
  int i = 0;
#ifdef DEBUG
  int rank, nproc;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (ap == NULL)          { return -1; }
  if (rp == NULL)          { return -1; }

  XMP_array_t = (_XMP_array_t*)ap;

  // �������Υ����å�
  if (XMP_array_t->dim != rp->dims) { return -1; }

#ifdef DEBUG
printf("WRITE(%d/%d) dims=%d\n",rank, nproc, rp->dims);
#endif

  /* �ѥå���ɬ�פʾ����̽��� */
  for (i = rp->dims - 1; i >= 0; i--){
     if(rp->step[i] < 0){
        int ret = xmp_fwrite_darray_pack(pstXmp_file, ap, rp);
        return ret;
     }
  }

  // ���ܥǡ������κ���
  MPI_Type_contiguous(XMP_array_t->type_size, MPI_BYTE, &dataType[0]);

  // ������ʬ�롼��
  for (i = rp->dims - 1; i >= 0; i--)
  {
#ifdef DEBUG
printf("WRITE(%d/%d) (lb,ub,step)=(%d,%d,%d)\n",
       rank, nproc, rp->lb[i],  rp->ub[i], rp->step[i]);
printf("WRITE(%d/%d) (par_lower,par_upper, par_size)=(%d,%d,%d)\n",
       rank, nproc, XMP_array_t->info[i].par_lower,
       XMP_array_t->info[i].par_upper, XMP_array_t->info[i].par_size);
printf("WRITE(%d/%d) (local_lower,local_upper,alloc_size)=(%d,%d,%d)\n",
       rank, nproc, XMP_array_t->info[i].local_lower,
        XMP_array_t->info[i].local_upper,  XMP_array_t->info[i].alloc_size);
printf("WRITE(%d/%d) (shadow_size_lo,shadow_size_hi)=(%d,%d)\n",
       rank, nproc, XMP_array_t->info[i].shadow_size_lo, XMP_array_t->info[i].shadow_size_hi);
#endif

    // ʬ����̵������
    if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
        XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION)
    {
      // ʬ����� < ����
      if (XMP_array_t->info[i].par_upper < rp->lb[i]) { return -1; }
      // ʬ��岼�� > ���
      if (XMP_array_t->info[i].par_lower > rp->ub[i]) { return -1; }

      // ��ʬ����
      if ( rp->step[i] < 0)
      {
      }
      // ��ʬ����
      else
      {
        // Ϣ³��Υ�����
        continuous_size = (rp->ub[i] - rp->lb[i]) / rp->step[i] + 1;

        // �ǡ��������ϰϤ����
        mpiRet =MPI_Type_get_extent(dataType[0], &tmp1, &tmp2);
        if (mpiRet !=  MPI_SUCCESS) { return -1; }  
        type_size = (int)tmp2;

        // ���ܥǡ�����������
        mpiRet = MPI_Type_create_hvector(continuous_size,
                                         1,
                                         type_size * rp->step[i],
                                         dataType[0],
                                         &dataType[1]);

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[0]);

        // MPI_Type_contiguous�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ���Υ�����
        total_size
          = (XMP_array_t->info[i].ser_upper
          -  XMP_array_t->info[i].ser_lower + 1)
          *  type_size;

        // ��֥�����
        space_size
          = (rp->lb[i] - XMP_array_t->info[i].par_lower)
          * type_size;

        // �������ե����뷿�κ���
        mpiRet = MPI_Type_create_resized(dataType[1],
                                         (MPI_Aint)space_size,
                                         (MPI_Aint)total_size,
                                         &dataType[0]);

        // MPI_Type_create_resized�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[1]);

#ifdef DEBUG
printf("WRITE(%d/%d) NOT_ALIGNED\n",rank, nproc);
printf("WRITE(%d/%d) type_size=%d\n",rank, nproc, type_size);
printf("WRITE(%d/%d) continuous_size=%d\n",rank, nproc, continuous_size);
printf("WRITE(%d/%d) space_size=%d\n",rank, nproc, space_size);
printf("WRITE(%d/%d) total_size=%d\n",rank, nproc, total_size);
#endif
      }
    }
     // blockʬ��
    else if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK)
    {
      // ��ʬ����
      if ( rp->step[i] < 0)
      {
      }
      // ��ʬ����
      else
      {
        // ʬ����� < ����
        if (XMP_array_t->info[i].par_upper < rp->lb[i])
        {
          continuous_size = 0;
        }
        // ʬ��岼�� > ���
        else if (XMP_array_t->info[i].par_lower > rp->ub[i])
        {
          continuous_size = 0;
        }
        // ����¾
        else
        {
          // �Ρ��ɤβ���
          lower
            = (XMP_array_t->info[i].par_lower > rp->lb[i]) ?
              rp->lb[i] + ((XMP_array_t->info[i].par_lower - 1 - rp->lb[i]) / rp->step[i] + 1) * rp->step[i]
            : rp->lb[i];

          // �Ρ��ɤξ��
          upper
            = (XMP_array_t->info[i].par_upper < rp->ub[i]) ?
               XMP_array_t->info[i].par_upper : rp->ub[i];

          // Ϣ³���ǿ�
          continuous_size = (upper - lower + rp->step[i]) / rp->step[i];
        }

        // �ǡ��������ϰϤ����
        mpiRet =MPI_Type_get_extent(dataType[0], &tmp1, &tmp2);
        if (mpiRet !=  MPI_SUCCESS) { return -1; }  
        type_size = (int)tmp2;
        if(lower > upper){
           type_size = 0;
        }

        // ���ܥǡ�����������
        mpiRet = MPI_Type_create_hvector(continuous_size,
                                         1,
                                         type_size * rp->step[i],
                                         dataType[0],
                                         &dataType[1]);

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[0]);

        // MPI_Type_create_hvector�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ��֥�����
        space_size
          = (XMP_array_t->info[i].local_lower
          + (lower - XMP_array_t->info[i].par_lower))
          * type_size;

        // ���Υ�����
        total_size = (XMP_array_t->info[i].alloc_size)* type_size;

        // �������ե����뷿�κ���
        mpiRet = MPI_Type_create_resized(dataType[1],
                                         (MPI_Aint)space_size,
                                         (MPI_Aint)total_size,
                                         &dataType[0]);

        // MPI_Type_create_resized�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return -1; }

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[1]);

#ifdef DEBUG
printf("WRITE(%d/%d) ALIGN_BLOCK\n",rank, nproc);
printf("WRITE(%d/%d) type_size=%d\n",rank, nproc, type_size);
printf("WRITE(%d/%d) continuous_size=%d\n",rank, nproc, continuous_size);
printf("WRITE(%d/%d) space_size=%d\n",rank, nproc, space_size);
printf("WRITE(%d/%d) total_size=%d\n",rank, nproc, total_size);
printf("WRITE(%d/%d) (lower,upper)=(%d,%d)\n",rank, nproc, lower, upper);
#endif
      }
    }
    // cyclicʬ��
    else if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_CYCLIC)
    {
      return -1;
    }
    // ����¾
    else
    {
      return -1;
    }
  }

  // ���ߥå�
  mpiRet = MPI_Type_commit(&dataType[0]);

  // ���ߥåȤ����顼�ξ��
  if (mpiRet != MPI_SUCCESS) { return 1; }
 
  // �����
  MPI_Type_size(dataType[0], &type_size);
  if(type_size > 0){
     if (MPI_File_write(pstXmp_file->fh,
                        (*XMP_array_t->array_addr_p),
                        1,
                        dataType[0],
                        &status)
         != MPI_SUCCESS)
        {
           return -1;
        }
  } else {
     return 0;
  }
 
  // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
  MPI_Type_free(&dataType[0]);

  // �ɹ�����Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &writeCount) != MPI_SUCCESS)
  {
    return -1;
  }
  return writeCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fread_shared                                         */
/*  DESCRIPTION   : ���δؿ��ϼ¹Ԥ����Ρ��ɤ��ͤ�ե�����ζ�ͭ�ե�����     */
/*                  �ݥ��󥿤ΰ��֤����ɹ��ࡣ                               */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  buffer[OUT] ��ʬ���ѿ�����Ƭ���ɥ쥹                     */
/*                  size[IN]  �ɹ���ǡ�����1��������Υ����� (�Х���)       */
/*                  count[IN] �ɹ���ǡ����ο�                               */
/*  RETURN VALUES : ���ｪλ�ξ����ɹ�����Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*****************************************************************************/
size_t xmp_fread_shared(xmp_file_t *pstXmp_file, void *buffer, size_t size, size_t count)
{
  MPI_Status status;
  int readCount;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (buffer      == NULL) { return -1; }
  if (size  < 1) { return -1; }
  if (count < 1) { return -1; }

  // �ɹ���
  if (MPI_File_read_shared(pstXmp_file->fh, buffer, size * count, MPI_BYTE, &status) != MPI_SUCCESS)
  {
    return -1;
  }
  
  // �ɹ�����Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &readCount) != MPI_SUCCESS)
  {
    return -1;
  }

  return readCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fwrite_shared                                        */
/*  DESCRIPTION   : ���δؿ��ϼ¹Ԥ����Ρ��ɤ��ͤ�ե�����ζ�ͭ�ե�����     */
/*                  �ݥ��󥿤ΰ��֤˽���ࡣ                                 */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  buffer[IN] ��ʬ���ѿ�����Ƭ���ɥ쥹                      */
/*                  size[IN]   �����ǡ�����1��������Υ����� (�Х���)      */
/*                  count[IN]  �����ǡ����ο�                              */
/*  RETURN VALUES : ���ｪλ�ξ��Ͻ������Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fwrite_shared(xmp_file_t *pstXmp_file, void *buffer, size_t size, size_t count)
{
  MPI_Status status;
  int writeCount;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (buffer      == NULL) { return -1; }
  if (size  < 1) { return -1; }
  if (count < 1) { return -1; }

  // �ե����륪���ץ�"r+"�ξ��Ͻ�ü�˥ݥ��󥿤��ư
  if (pstXmp_file->is_append)
  {
    if (MPI_File_seek_shared(pstXmp_file->fh,
                             (MPI_Offset)0,
                             MPI_SEEK_END) != MPI_SUCCESS)
    {
      return -1;
    }

    pstXmp_file->is_append = 0x00;
  }

  // �����
  if (MPI_File_write_shared(pstXmp_file->fh,
                            buffer,
                            size * count,
                            MPI_BYTE,
                            &status) != MPI_SUCCESS)
  {
    return -1;
  }

  // �������Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &writeCount) != MPI_SUCCESS)
  {
    return -1;
  }

  return writeCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fread                                                */
/*  DESCRIPTION   : ���δؿ��ϼ¹Ԥ����Ρ��ɤ�buffer�إե�����ӥ塼�˽���   */
/*                  �ǡ������ɹ��ࡣ                                         */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  buffer[OUT] ��ʬ���ѿ�����Ƭ���ɥ쥹                     */
/*                  size[IN]    �ɹ���ǡ�����1��������Υ����� (�Х���)     */
/*                  count[IN]   �ɹ���ǡ����ο�                             */
/*  RETURN VALUES : ���ｪλ�ξ����ɹ�����Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fread(xmp_file_t *pstXmp_file, void *buffer, size_t size, size_t count)
{
  MPI_Status status;
  int readCount;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (buffer      == NULL) { return -1; }
  if (size  < 1) { return -1; }
  if (count < 1) { return -1; }

  // �ɹ���
  if (MPI_File_read(pstXmp_file->fh, buffer, size * count, MPI_BYTE, &status) != MPI_SUCCESS)
  {
    return -1;
  }
  
  // �ɹ�����Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &readCount) != MPI_SUCCESS)
  {
    return -1;
  }

  return readCount;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_fwrite                                               */
/*  DESCRIPTION   : ���δؿ��ϼ¹Ԥ����Ρ��ɤ�buffer����ե�����ӥ塼��     */
/*                  �����ǡ��������ࡣ                                     */
/*                  ���δؿ��ϥ�����¹Բ�ǽ�Ǥ��롣                       */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  buffer[IN] ��ʬ���ѿ�����Ƭ���ɥ쥹                      */
/*                  size[IN]   �����ǡ�����1��������Υ����� (�Х���)      */
/*                  count[IN]  �����ǡ����ο�                              */
/*  RETURN VALUES : ���ｪλ�ξ��Ͻ������Х��ȿ����֤���                 */
/*                  �۾ｪλ�ξ���������֤���                             */
/*                                                                           */
/*****************************************************************************/
size_t xmp_fwrite(xmp_file_t *pstXmp_file, void *buffer, size_t size, size_t count)
{
  MPI_Status status;
  int writeCount;

  // ���������å�
  if (pstXmp_file == NULL) { return -1; }
  if (buffer      == NULL) { return -1; }
  if (size  < 1) { return -1; }
  if (count < 1) { return -1; }

  // �ե����륪���ץ�"r+"�ξ��Ͻ�ü�˥ݥ��󥿤��ư
  if (pstXmp_file->is_append)
  {
    if (MPI_File_seek(pstXmp_file->fh,
                      (MPI_Offset)0,
                      MPI_SEEK_END) != MPI_SUCCESS)
    {
      return -1;
    }

    pstXmp_file->is_append = 0x00;
  }

  // �����
  if (MPI_File_write(pstXmp_file->fh,
                     buffer,
                     size * count,
                     MPI_BYTE,
                     &status) != MPI_SUCCESS)
  {
    return -1;
  }

  // �������Х��ȿ�
  if (MPI_Get_count(&status, MPI_BYTE, &writeCount) != MPI_SUCCESS)
  {
    return -1;
  }

  return writeCount;
}


/*****************************************************************************/
/*  FUNCTION NAME : xmp_file_set_view                                        */
/*  DESCRIPTION   : ���δؿ���ap�ǻ��ꤵ���ʬ������ˤĤ��ơ�rp�ǻ��ꤵ��� */
/*                  �ϰϤΥե�����ӥ塼��fh�����ꤹ�롣                     */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  disp[IN] �ե�������Ƭ������Ѱ� (�Х���)                 */
/*                  ap[IN]   ʬ���������                                    */
/*                  rp[IN]   ���������ϰϾ���                                */
/*  RETURN VALUES : ���ｪλ�ξ���0���֤���                                */
/*                  �۾ｪλ�ξ���0�ʳ����ͤ��֤���                        */
/*                                                                           */
/*****************************************************************************/
int xmp_file_set_view_all(xmp_file_t  *pstXmp_file,
                          long long    disp,
                          xmp_array_t  ap,
                          xmp_range_t *rp)
{
  _XMP_array_t *XMP_array_t;
  int i = 0;
  int mpiRet;               // MPI�ؿ������
  int lower;                // ���ΥΡ��ɤǥ����������벼��
  int upper;                // ���ΥΡ��ɤǥ�������������
  int continuous_size;      // Ϣ³�襵����
  MPI_Datatype dataType[2];
  int type_size;
  MPI_Aint tmp1, tmp2;
#ifdef DEBUG
  int rank, nproc;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif

  // ���������å�
  if (pstXmp_file == NULL) { return 1; }
  if (ap == NULL)          { return 1; }
  if (rp == NULL)          { return 1; }
  if (disp  < 0)           { return 1; }

  XMP_array_t = (_XMP_array_t*)ap; 

  // �������Υ����å�
  if (XMP_array_t->dim != rp->dims) { return 1; }

#ifdef DEBUG
printf("VIEW(%d/%d) dims=%d\n", rank, nproc, rp->dims);
#endif

  // ���ܥǡ������κ���
  MPI_Type_contiguous(XMP_array_t->type_size, MPI_BYTE, &dataType[0]);

  // ������ʬ�롼��
  for (i = rp->dims - 1; i >= 0; i--)
  {
    // �ǡ��������ϰϤ����
    mpiRet =MPI_Type_get_extent(dataType[0], &tmp1, &tmp2);
    if (mpiRet !=  MPI_SUCCESS) { return -1; }  
    type_size = (int)tmp2;

#ifdef DEBUG
printf("VIEW(%d/%d) (lb,ub,step)=(%d,%d,%d)\n",
        rank, nproc, rp->lb[i],  rp->ub[i], rp->step[i]);
printf("VIEW(%d/%d) (par_lower,par_upper)=(%d,%d)\n",
        rank, nproc, XMP_array_t->info[i].par_lower, XMP_array_t->info[i].par_upper);
#endif
    // ʬ����̵������
    if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_NOT_ALIGNED ||
        XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_DUPLICATION)
    {
      // Ϣ³��Υ�����
      continuous_size = (rp->ub[i] - rp->lb[i]) / rp->step[i] + 1;

      // ���ܥǡ�����������
      mpiRet = MPI_Type_contiguous(continuous_size, dataType[0], &dataType[1]);

      // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
      MPI_Type_free(&dataType[0]);
      dataType[0] = dataType[1];

      // MPI_Type_contiguous�����顼�ξ��
      if (mpiRet != MPI_SUCCESS) { return 1; }

#ifdef DEBUG
printf("VIEW(%d/%d) NOT_ALIGNED\n", rank, nproc);
printf("VIEW(%d/%d) continuous_size=%d\n", rank, nproc, continuous_size);
#endif
    }
    // blockʬ��
    else if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_BLOCK)
    {
      int space_size;
      int total_size;

      // ��ʬ�����ξ��
      if (rp->step[i] >= 0)
      {
        // ���� > ���
        if (rp->lb[i] > rp->ub[i])
        {
          return 1;
        }
        // ʬ����� < ����
        else if (XMP_array_t->info[i].par_upper < rp->lb[i])
        {
          continuous_size = space_size = 0;
        }
        // ʬ��岼�� > ���
        else if (XMP_array_t->info[i].par_lower > rp->ub[i])
        {
          continuous_size = space_size = 0;
        }
        // ����¾
        else
        {
          // �Ρ��ɤβ���
          lower
            = (XMP_array_t->info[i].par_lower > rp->lb[i]) ?
              rp->lb[i] + ((XMP_array_t->info[i].par_lower - 1 - rp->lb[i]) / rp->step[i] + 1) * rp->step[i]
            : rp->lb[i];

          // �Ρ��ɤξ��
          upper
            = (XMP_array_t->info[i].par_upper < rp->ub[i]) ?
               XMP_array_t->info[i].par_upper : rp->ub[i];

          // Ϣ³���ǿ�
          continuous_size = (upper - lower) / rp->step[i] + 1;

          // ��֥�����
          space_size
            = ((lower - rp->lb[i]) / rp->step[i]) * type_size;
        }

        // ���Υ�����
        total_size
          = ((rp->ub[i] - rp->lb[i]) / rp->step[i] + 1) * type_size;

        // ���ܥǡ�����������
        mpiRet = MPI_Type_contiguous(continuous_size, dataType[0], &dataType[1]);

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[0]);

        // MPI_Type_contiguous�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return 1; }

        // �������ե����뷿�κ���
        mpiRet = MPI_Type_create_resized(dataType[1],
                                         space_size,
                                         total_size,
                                         &dataType[0]);

        // MPI_Type_create_resized�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return 1; }


        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[1]);

#ifdef DEBUG
printf("VIEW(%d/%d) ALIGN_BLOCK\n", rank, nproc );
printf("VIEW(%d/%d) type_size=%d\n", rank, nproc , type_size);
printf("VIEW(%d/%d) continuous_size=%d\n", rank, nproc , continuous_size);
printf("VIEW(%d/%d) space_size=%d\n", rank, nproc , space_size);
printf("VIEW(%d/%d) total_size=%d\n", rank, nproc , total_size);
printf("VIEW(%d/%d) (lower,upper)=(%d,%d)\n", rank, nproc , lower, upper);
printf("\n");
#endif
      }
      // ��ʬ����ξ��
      else if (rp->step[i] < 0)
      {
        // ���� < ���
        if (rp->lb[i] < rp->ub[i])
        {
          return 1;
        }
        // ʬ��岼�� < ���
        else if (XMP_array_t->info[i].par_lower < rp->ub[i])
        {
          continuous_size = space_size = 0;
        }
        // ʬ����� > ����
        else if (XMP_array_t->info[i].par_upper > rp->lb[i])
        {
          continuous_size = space_size = 0;
        }
        // ����¾
        else
        {
          // �Ρ��ɤβ���
          lower
            = (XMP_array_t->info[i].par_upper <  rp->lb[i]) ?
              rp->lb[i] - (( rp->lb[i] - XMP_array_t->info[i].par_upper - 1) / rp->step[i] - 1) * rp->step[i]
            : rp->lb[i];

          // �Ρ��ɤξ��
          upper
            = (XMP_array_t->info[i].par_lower > rp->ub[i]) ?
               XMP_array_t->info[i].par_lower : rp->ub[i];

          // Ϣ³���ǿ�
          continuous_size = (upper - lower) / rp->step[i] + 1;

          // ��֥�����
          space_size
            = ((lower - rp->lb[i]) / rp->step[i]) * type_size;
        }

        // ���ܥǡ�����������
        mpiRet = MPI_Type_contiguous(continuous_size, dataType[0], &dataType[1]);

        // ���Υ�����
        total_size
          = ((rp->ub[i] - rp->lb[i]) / rp->step[i] + 1) * type_size;

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[0]);

        // MPI_Type_contiguous�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return 1; }

        // �������ե����뷿�κ���
        mpiRet = MPI_Type_create_resized(dataType[1],
                                         space_size,
                                         total_size,
                                         &dataType[0]);

        // MPI_Type_create_resized�����顼�ξ��
        if (mpiRet != MPI_SUCCESS) { return 1; }

        // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
        MPI_Type_free(&dataType[1]);

#ifdef DEBUG
printf("VIEW(%d/%d) ALIGN_BLOCK\n", rank, nproc);
printf("VIEW(%d/%d) continuous_size=%d\n", rank, nproc, continuous_size);
printf("VIEW(%d/%d) space_size=%d\n", rank, nproc, space_size);
printf("VIEW(%d/%d) total_size=%d\n", rank, nproc, total_size);
printf("VIEW(%d/%d) (lower,upper)=(%d,%d)\n", rank, nproc, lower, upper);
#endif
      }
    }
    // cyclicʬ��
    else if (XMP_array_t->info[i].align_manner == _XMP_N_ALIGN_CYCLIC)
    {
      return 1;
    }
    // ����¾
    else
    {
      return 1;
    }
  }

  // ���ߥå�
  mpiRet = MPI_Type_commit(&dataType[0]);

  // ���ߥåȤ����顼�ξ��
  if (mpiRet != MPI_SUCCESS) { return 1; }
  
  // �ӥ塼�Υ��å�
  mpiRet = MPI_File_set_view(pstXmp_file->fh,
                             (MPI_Offset)disp,
                             MPI_BYTE,
                             dataType[0],
                             "native",
                             MPI_INFO_NULL);


  // ���Ѥ��ʤ��ʤä�MPI_Datatype�����
  //MPI_Type_free(&dataType[0]);

  // �ӥ塼�Υ��åȤ����顼�ξ��
  if (mpiRet != MPI_SUCCESS) { return 1; }

  return 0;
}

/*****************************************************************************/
/*  FUNCTION NAME : xmp_file_clear_view                                      */
/*  DESCRIPTION   : ���δؿ��ϥե�����ӥ塼���������롣����������ȡ�   */
/*                  �ƥե�����ݥ��󥿤�disp�����ꤵ�졢���ǥǡ�������       */
/*                  �ե����뷿��MPI_BYTE�����ꤵ��롣                       */
/*                  ���δؿ��Ͻ��ļ¹Ԥ��ʤ���Фʤ�ʤ���                   */
/*  ARGUMENT      : pstXmp_file[IN] �ե����빽¤��                           */
/*                  disp[IN] �ե�������Ƭ������Ѱ� (�Х���)                 */
/*  RETURN VALUES : ���ｪλ�ξ���0���֤���                                */
/*                  �۾ｪλ�ξ���0�ʳ����ͤ��֤���                        */
/*                                                                           */
/*****************************************************************************/
int xmp_file_clear_view_all(xmp_file_t  *pstXmp_file, long long disp)
{
  // ���������å�
  if (pstXmp_file == NULL) { return 1; }
  if (disp  < 0)           { return 1; }

  // �ӥ塼�ν����
  if (MPI_File_set_view(pstXmp_file->fh,
                        disp,
                        MPI_BYTE,
                        MPI_BYTE,
                        "native",
                        MPI_INFO_NULL) != MPI_SUCCESS)
  {
    return 1;
  }

  return 0;
}

/*****************************************************************************/
/*  FUNCTION NAME : MPI_Type_create_resized                                  */
/*                                                                           */
/*****************************************************************************/
int MPI_Type_create_resized(MPI_Datatype oldtype,
                            MPI_Aint     lb,
                            MPI_Aint     extent,
                            MPI_Datatype *newtype)
{
        int          mpiRet;
        int          b[3];
        MPI_Aint     d[3];
        MPI_Datatype t[3];

        b[0] = b[1] = b[2] = 1;
        d[0] = 0;
        d[1] = lb;
        d[2] = extent;
        t[0] = MPI_LB;
        t[1] = oldtype;
        t[2] = MPI_UB;

        mpiRet = MPI_Type_create_struct(3, b, d, t, newtype);

        return mpiRet;
}
