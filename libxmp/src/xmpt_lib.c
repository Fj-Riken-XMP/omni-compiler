#include "xmp_internal.h"

xmpt_callback_t xmpt_callback[XMPT_EVENT_ALL] = { 0 };
int xmpt_support_level[XMPT_EVENT_ALL] = {
  1, /* xmpt_event_task_begin */
  1, /* xmpt_event_task_end */
  2, /* xmpt_event_tasks_begin */
  2, /* xmpt_event_tasks_end */
  1, /* xmpt_event_loop_begin */
  1, /* xmpt_event_loop_end */
  2, /* xmpt_event_array_begin */
  2, /* xmpt_event_array_end */
  4, /* xmpt_event_reflect_begin */
  4, /* xmpt_event_reflect_begin_async */
  4, /* xmpt_event_reflect_end */
  3, /* xmpt_event_gmove_begin */
  3, /* xmpt_event_gmove_begin_async */
  4, /* xmpt_event_gmove_end */
  1, /* xmpt_event_barrier_begin */
  1, /* xmpt_event_barrier_end */
  1, /* xmpt_event_reduction_begin */
  1, /* xmpt_event_reduction_begin_async */
  1, /* xmpt_event_reduction_end */
  3, /* xmpt_event_bcast_begin */
  3, /* xmpt_event_bcast_begin_async */
  4, /* xmpt_event_bcast_end */
  3, /* xmpt_event_wait_async_begin */
  3, /* xmpt_event_wait_async_end */
  3, /* xmpt_event_coarray_remote_write */
  3, /* xmpt_event_coarray_remote_read */
  3, /* xmpt_event_coarray_local_write */
  3, /* xmpt_event_coarray_local_read */
  4, /* xmpt_event_sync_memory_begin */
  4, /* xmpt_event_sync_memory_end */
  4, /* xmpt_event_sync_all_begin */
  4, /* xmpt_event_sync_all_end */
  2, /* xmpt_event_sync_image_begin */
  2, /* xmpt_event_sync_image_end */
  2, /* xmpt_event_sync_images_all_begin */
  2, /* xmpt_event_sync_images_all_end */
  4, /* xmpt_event_sync_images_begin */
  4, /* xmpt_event_sync_images_end */
  0  /* XMPT_EVENT_ALL */
};

int __attribute__((weak)) xmpt_initialize(){
  return 0;
}

xmp_desc_t on_desc;
struct _xmpt_subscript_t on_subsc;
xmp_desc_t from_desc;
struct _xmpt_subscript_t from_subsc;

int xmpt_set_callback(xmpt_event_t event, xmpt_callback_t callback){

  /* 0 callback registration error (e.g., callbacks cannot be registered at this time). */
  /* 1 event may occur; no callback is possible (e.g., not yet implemented) */
  /* 2 event will never occur in this runtime */
  /* 3 event may occur; callback invoked when convenient */
  /* 4 event may occur; callback always invoked when event occurs */

  if (!callback || event < 0 || event >= XMPT_EVENT_ALL) return 0;

  if (xmpt_support_level[event] >= 3){
    xmpt_callback[event] = callback;
  }

  return xmpt_support_level[event];

  /* if (event < XMPT_EVENT_FULLY_SUPPORTED){ */
  /*   xmpt_callback[event] = callback; */
  /*   return 4; */
  /* } */
  /* else if (event < XMPT_EVENT_PARTIALLY_SUPPORTED){ */
  /*   xmpt_callback[event] = callback; */
  /*   return 3; */
  /* } */
  /* else if (event < XMPT_EVENT_NEVER){ */
  /*   return 2; */
  /* } */
  /* else if (event < XMPT_EVENT_ALL){ */
  /*   return 1; */
  /* } */
  /* else { // == XMPT_EVENT_XXX */
  /*   return 0; */
  /* } */

}


void _XMP_loop_begin(void *desc, xmpt_tool_data_t *data, ...){

  struct _xmpt_subscript_t subsc;
  if (xmpt_enabled && xmpt_callback[xmpt_event_loop_begin]){
    subsc.ndims = ((_XMP_template_t *)desc)->dim;
    subsc.omit = 0;
    va_list args;
    va_start(args, data);
    for (int i = 0; i < subsc.ndims; i++){
      subsc.lbound[i] = va_arg(args, int);
      subsc.ubound[i] = va_arg(args, int);
      subsc.marker[i] = va_arg(args, int);
    }
    va_end(args);
    (*(xmpt_event_single_desc_begin_t)xmpt_callback[xmpt_event_loop_begin])(
     desc, &subsc, data);
  }

}

void _XMP_loop_end(xmpt_tool_data_t *data){

  if (xmpt_enabled && xmpt_callback[xmpt_event_loop_end]){
    (*(xmpt_event_end_t)xmpt_callback[xmpt_event_loop_end])(
     data);
  }

}

void _XMP_coarray_local_read(void *local_coarray, ...){

  struct _xmpt_subscript_t subsc;
  xmpt_tool_data_t *data = NULL;
  if (xmpt_enabled && xmpt_callback[xmpt_event_coarray_local_read]){
    subsc.ndims = ((_XMP_coarray_t*)local_coarray)->coarray_dims;
    subsc.omit = 0;
    va_list args;
    va_start(args, local_coarray);
    for (int i = 0; i < subsc.ndims; i++){
      subsc.lbound[i] = va_arg(args, int);
      subsc.ubound[i] = va_arg(args, int);
      subsc.marker[i] = va_arg(args, int);
    }
    va_end(args);
    (*(xmpt_event_coarray_local_t)xmpt_callback[xmpt_event_coarray_local_read])(
     (xmpt_coarray_id_t)local_coarray, &subsc, data);
  }
}

void _XMP_coarray_local_write(void *local_coarray, ...){

  struct _xmpt_subscript_t subsc;
  xmpt_tool_data_t *data = NULL;
  if (xmpt_enabled && xmpt_callback[xmpt_event_coarray_local_write]){
    subsc.ndims = ((_XMP_coarray_t*)local_coarray)->coarray_dims;
    subsc.omit = 0;
    va_list args;
    va_start(args, local_coarray);
    for (int i = 0; i < subsc.ndims; i++){
      subsc.lbound[i] = va_arg(args, int);
      subsc.ubound[i] = va_arg(args, int);
      subsc.marker[i] = va_arg(args, int);
    }
    va_end(args);
    (*(xmpt_event_coarray_local_t)xmpt_callback[xmpt_event_coarray_local_write])(
     (xmpt_coarray_id_t)local_coarray, &subsc, data);
  }
}

int xmpt_coarray_get_gid(xmpt_coarray_id_t c){
  return ((_XMP_coarray_t*)c)->gid;
}

/* void _XMPT_set_on_desc(void *local_coarray, ...){ */
/* } */

/* void _XMPT_set_from_desc(void *local_coarray, ...){ */
/* } */

void _XMPT_set_gmove_subsc(xmpt_subscript_t subsc, _XMP_gmv_desc_t *gmv_desc){
  subsc->ndims = gmv_desc->ndims;
  subsc->omit = 0;
  for (int i = 0; i < subsc->ndims; i++){
    subsc->lbound[i] = gmv_desc->lb[i];
    subsc->ubound[i] = gmv_desc->ub[i];
    subsc->marker[i] = gmv_desc->st[i];
  }
}

void _XMPT_set_bcast_subsc(xmpt_subscript_t subsc, _XMP_object_ref_t *desc){
  if (desc){
    subsc->ndims = desc->ndims;
    subsc->omit = 0;
    for (int i = 0; i < subsc->ndims; i++){
      subsc->lbound[i] = desc->REF_LBOUND[i];
      subsc->ubound[i] = desc->REF_UBOUND[i];
      subsc->marker[i] = desc->REF_STRIDE[i];
    }
  }
  else {
    subsc->omit = 1;
  }
}
