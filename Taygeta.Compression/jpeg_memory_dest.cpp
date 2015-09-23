// Taken from the Galapix project
//
// http://code.google.com/p/galapix/
//
//

#include "stdafx.h"
#include <iostream>
#include "jpeg_memory_dest.h"

#define OUTPUT_BUF_SIZE 4096

struct jpeg_memory_destination_mgr 
{
  struct jpeg_destination_mgr pub;

  JOCTET buffer[OUTPUT_BUF_SIZE];
  std::vector<BYTE>* data;
};

void jpeg_memory_init_destination(j_compress_ptr cinfo)
{
  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;

  cinfo->dest->next_output_byte = mgr->buffer;
  cinfo->dest->free_in_buffer   = OUTPUT_BUF_SIZE;
}

boolean jpeg_memory_empty_output_buffer(j_compress_ptr cinfo)
{
  //std::cout << "jpeg_memory_empty_output_buffer(j_compress_ptr cinfo)" << std::endl;

  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;
  
  // This function always gets OUTPUT_BUF_SIZE bytes,
  // cinfo->dest->free_in_buffer *must* be ignored
  for(size_t i = 0; i < OUTPUT_BUF_SIZE; ++i) 
    { // Little slow maybe?
      mgr->data->push_back(mgr->buffer[i]);
    }

  cinfo->dest->next_output_byte = mgr->buffer;
  cinfo->dest->free_in_buffer   = OUTPUT_BUF_SIZE;

  return TRUE;
}

void jpeg_memory_term_destination(j_compress_ptr cinfo)
{
  //std::cout << "jpeg_memory_term_destination)" << std::endl;

  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - cinfo->dest->free_in_buffer;

  for(size_t i = 0; i < datacount; ++i)
    { // Little slow maybe?
      mgr->data->push_back(mgr->buffer[i]);
    } 
}

void jpeg_memory_dest(j_compress_ptr cinfo, std::vector<BYTE>* data)
{
  if (cinfo->dest == NULL) 
    {   /* first time for this JPEG object? */
      cinfo->dest = (struct jpeg_destination_mgr*)
        (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                   sizeof(struct jpeg_memory_destination_mgr));
    }

  cinfo->dest->init_destination    = jpeg_memory_init_destination;
  cinfo->dest->empty_output_buffer = jpeg_memory_empty_output_buffer;
  cinfo->dest->term_destination    = jpeg_memory_term_destination;

  struct jpeg_memory_destination_mgr* mgr = (struct jpeg_memory_destination_mgr*)cinfo->dest;
  mgr->data = data;
}