#include "gt_gtf.h"

GT_INLINE gt_gtf_entry* gt_gtf_entry_new(const uint64_t start, const uint64_t end, const gt_strand strand, gt_string* const type){
  gt_gtf_entry* entry = malloc(sizeof(gt_gtf_entry));
  entry->uid = 0;
  entry->start = start;
  entry->end = end;
  entry->type = type;
  entry->strand = strand;
  return entry;
}
GT_INLINE void gt_gtf_entry_delete(gt_gtf_entry* const entry){
  free(entry);
}

GT_INLINE gt_gtf_ref* gt_gtf_ref_new(void){
  gt_gtf_ref* ref = malloc(sizeof(gt_gtf_ref));
  ref->entries = gt_vector_new(GTF_DEFAULT_ENTRIES, sizeof(gt_gtf_entry*));
  return ref;
}
GT_INLINE void gt_gtf_ref_delete(gt_gtf_ref* const ref){
  register uint64_t s = gt_vector_get_used(ref->entries);
  register uint64_t i = 0;
  for(i=0; i<s; i++){
    gt_gtf_entry_delete( (gt_vector_get_elm(ref->entries, i, gt_gtf_entry)));
  }
  gt_vector_delete(ref->entries);
  free(ref);
}

GT_INLINE gt_gtf* gt_gtf_new(void){
  gt_gtf* gtf = malloc(sizeof(gt_gtf));
  gtf->refs = gt_shash_new();
  gtf->types = gt_shash_new();
  gtf->gene_ids = gt_shash_new();
  gtf->transcript_ids = gt_shash_new();
  gtf->gene_types = gt_shash_new();
  return gtf;
}

GT_INLINE void gt_gtf_delete(gt_gtf* const gtf){
  gt_shash_delete(gtf->refs, true);
  gt_shash_delete(gtf->types, true);
  gt_shash_delete(gtf->gene_ids, true);
  gt_shash_delete(gtf->transcript_ids, true);
  gt_shash_delete(gtf->gene_types, true);
  free(gtf);
}

GT_INLINE gt_gtf_hits* gt_gtf_hits_new(void){
  gt_gtf_hits* hits = malloc(sizeof(gt_gtf_hits));
  hits->exon_hits = gt_vector_new(16, sizeof(gt_gtf_hit*));
  return hits;
}
GT_INLINE void gt_gtf_hits_delete(gt_gtf_hits* const hits){
  gt_gtf_hits_clear(hits);
  gt_vector_delete(hits->exon_hits);
  free(hits);
}
GT_INLINE void gt_gtf_hits_clear(gt_gtf_hits* const hits){
  uint64_t i = 0;
  for(i=0; i<gt_vector_get_used(hits->exon_hits); i++){
    gt_gtf_hit* hit = *gt_vector_get_elm(hits->exon_hits, i, gt_gtf_hit*);
    gt_gtf_hit_delete(hit);
  }
  gt_vector_clear(hits->exon_hits);
}


GT_INLINE gt_string* gt_gtf_get_type(const gt_gtf* const gtf, char* const type){
  if(!gt_gtf_contains_type(gtf, type)){
    gt_string* s = gt_string_set_new(type);
    gt_shash_insert_string(gtf->types, type, s);
  }
  return gt_shash_get(gtf->types, type, gt_string);
}
GT_INLINE bool gt_gtf_contains_type(const gt_gtf* const gtf, char* const name){
	return gt_shash_is_contained(gtf->types, name);
}

GT_INLINE gt_gtf_ref* gt_gtf_get_ref(const gt_gtf* const gtf, char* const name){
  if(!gt_gtf_contains_ref(gtf, name)){
    gt_gtf_ref* rr = gt_gtf_ref_new();
    gt_shash_insert(gtf->refs, name, rr, gt_gtf_ref*);
  }
  return gt_shash_get(gtf->refs, name, gt_gtf_ref);
}
GT_INLINE bool gt_gtf_contains_ref(const gt_gtf* const gtf, char* const name){
	return gt_shash_is_contained(gtf->refs, name);
}

GT_INLINE gt_string* gt_gtf_get_gene_id(const gt_gtf* const gtf, char* const name){
  if(!gt_gtf_contains_gene_id(gtf, name)){
    gt_string* const gene_id = gt_string_set_new(name);
    gt_shash_insert(gtf->gene_ids, name, gene_id, gt_string*);
  }
  return gt_shash_get(gtf->gene_ids, name, gt_string);
}
GT_INLINE bool gt_gtf_contains_gene_id(const gt_gtf* const gtf, char* const name){
	return gt_shash_is_contained(gtf->gene_ids, name);
}

GT_INLINE gt_string* gt_gtf_get_transcript_id(const gt_gtf* const gtf, char* const name){
  if(!gt_gtf_contains_transcript_id(gtf, name)){
    gt_string* const gene_id = gt_string_set_new(name);
    gt_shash_insert(gtf->transcript_ids, name, gene_id, gt_string*);
  }
  return gt_shash_get(gtf->transcript_ids, name, gt_string);
}
GT_INLINE bool gt_gtf_contains_transcript_id(const gt_gtf* const gtf, char* const name){
	return gt_shash_is_contained(gtf->transcript_ids, name);
}

GT_INLINE gt_string* gt_gtf_get_gene_type(const gt_gtf* const gtf, char* const name){
  if(!gt_gtf_contains_gene_type(gtf, name)){
    gt_string* const gene_type = gt_string_set_new(name);
    gt_shash_insert(gtf->gene_types, name, gene_type, gt_string*);
  }
  return gt_shash_get(gtf->gene_types, name, gt_string);
}
GT_INLINE bool gt_gtf_contains_gene_type(const gt_gtf* const gtf, char* const name){
	return gt_shash_is_contained(gtf->gene_types, name);
}

/**
 * Comparator that compares two gtf_entries by starting position
 */
GT_INLINE int gt_gtf_sort_by_start_cmp_(const gt_gtf_entry** a, const gt_gtf_entry** b){
  uint64_t p1 = (*a)->start;
  uint64_t p2 = (*b)->start;
  return p1 < p2 ? -1 : (p1>p2 ? 1 : gt_string_cmp( (*a)->type, (*b)->type ));
}
/**
 * Comparator that compares two gtf_entries by ending position
 */
GT_INLINE int gt_gtf_sort_by_end_cmp_(const gt_gtf_entry** a, const gt_gtf_entry** b){
  uint64_t p1 = (*a)->end;
  uint64_t p2 = (*b)->end;
  return p1 < p2 ? -1 : (p1>p2 ? 1 : gt_string_cmp( (*a)->type, (*b)->type ));
}

/**
 * Sort vector of gt_gtf_entries by starting position
 */
GT_INLINE void gt_gtf_sort_by_start(gt_vector* entries) {
  qsort(gt_vector_get_mem(entries, gt_gtf_entry*),
    gt_vector_get_used(entries),
    sizeof(gt_gtf_entry**),
    (int (*)(const void *,const void *))gt_gtf_sort_by_start_cmp_);
}
/**
 * Sort vector of gt_gtf_entries by ending position
 */
GT_INLINE void gt_gtf_sort_by_end( gt_vector* entries) {
  qsort(gt_vector_get_mem(entries, gt_gtf_entry*),
    gt_vector_get_used(entries),
    sizeof(gt_gtf_entry**),
    (int (*)(const void *,const void *))gt_gtf_sort_by_end_cmp_);
}
GT_INLINE gt_gtf_node* gt_gtf_create_node(gt_vector* entries){
  const uint64_t len = gt_vector_get_used(entries);
  if(len == 0){
    return NULL;
  }
  gt_gtf_node* const node = malloc(sizeof(gt_gtf_node));
  const gt_gtf_entry* mid = *gt_vector_get_elm(entries, len/2, gt_gtf_entry*);
  node->midpoint = mid->start + ((mid->end - mid->start)/2);
  node->entries_by_end = gt_vector_new(16, sizeof(gt_gtf_entry*));
  node->entries_by_start = gt_vector_new(16, sizeof(gt_gtf_entry*));
  gt_vector* to_left = gt_vector_new(16, sizeof(gt_gtf_entry*));
  gt_vector* to_right = gt_vector_new(16, sizeof(gt_gtf_entry*));
  GT_VECTOR_ITERATE(entries, element, counter, gt_gtf_entry*){
    if((*element)->end < node->midpoint){
      gt_vector_insert(to_left, (*element), gt_gtf_entry*);
    }else if((*element)->start > node->midpoint){
      gt_vector_insert(to_right, (*element), gt_gtf_entry*);
    }else{
      gt_vector_insert(node->entries_by_end, (*element), gt_gtf_entry*);
      gt_vector_insert(node->entries_by_start, (*element), gt_gtf_entry*);
    }
  }
  // sort the start and end lists
  gt_gtf_sort_by_start(node->entries_by_start);
  gt_gtf_sort_by_end(node->entries_by_end);

  // delete incoming entry list
  gt_vector_delete(entries);
  if(gt_vector_get_used(to_left) > 0){
    // create left node
    node->left = gt_gtf_create_node(to_left);
  }else{
    node->left = NULL;
    gt_vector_delete(to_left);
  }
  if(gt_vector_get_used(to_right) > 0){
    // create right node
    node->right = gt_gtf_create_node(to_right);
  }else{
    node->right = NULL;
    gt_vector_delete(to_right);
  }
  return node;
}

/**
 * Parse a single GTF line
 */
GT_INLINE void gt_gtf_read_line(char* line, gt_gtf* const gtf, uint64_t counter){
  // skip comments
  if(line[0] == '#'){
    return;
  }
  char* ref = NULL;
  char* type = NULL;
  char* gene_id = NULL;
  char* transcript_id = NULL;
  char* gene_type = NULL;
  uint64_t start = 0;
  uint64_t end = 0;

  gt_strand strand = UNKNOWN;

  char * pch;
  // name
  pch = strtok(line, "\t");
  if(pch == NULL){
    return;
  }
  ref = pch;
  // source
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  // type
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  type = pch;
  // start
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  start = atol(pch);
  // end
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  end = atol(pch);
  // score
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  // strand
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  if(pch[0] == '+'){
    strand = FORWARD;
  }else if(pch[0] == '-'){
    strand = REVERSE;
  }
  // last thing where i can not remember what it was
  pch = strtok(NULL, "\t");
  if(pch == NULL){
    return;
  }
  // additional fields
  // search for gene_id
  register bool gid = false;
  register bool gene_t = false;
  register bool transcript_t = false;
  while((pch = strtok(NULL, " ")) != NULL){
    //printf(" PCH: %s\n", pch);
    //if(gene_id != NULL && transcript_id != NULL && gene_type != NULL) break;
    if(strcmp("gene_id", pch) == 0){
      gid = true;
    }else if(strcmp("gene_type", pch) == 0){
      gene_t = true;
    }else if(strcmp("transcript_id", pch) == 0){
      transcript_t = true;
    }else{
      if(gid){
        gid = false;
        gene_id = pch;
        register uint64_t l = strlen(gene_id);
        register uint64_t off = 1;
        if(gene_id[l-off] == ';'){
          gene_id[l-off] = '\0';
          off = 2;
        }
        if(gene_id[0] == '"'){
          gene_id++;
          gene_id[l-(off+1)] = '\0';
        }
      }else if(gene_t){
        gene_t = false;
        gene_type = pch;
        register uint64_t l = strlen(gene_type);
        register uint64_t off = 1;
        if(gene_type[l-off] == ';'){
          gene_type[l-off] = '\0';
          off = 2;
        }
        if(gene_type[0] == '"'){
          gene_type++;
          gene_type[l-(off+1)] = '\0';
        }
      }else if(transcript_t){
        transcript_t = false;
        transcript_id = pch;
        register uint64_t l = strlen(transcript_id);
        register uint64_t off = 1;
        if(transcript_id[l-off] == ';'){
          transcript_id[l-off] = '\0';
          off = 2;
        }
        if(transcript_id[0] == '"'){
          transcript_id++;
          transcript_id[l-(off+1)] = '\0';
        }
      }
    }
  }
  // get the type or create it
  gt_string* tp = gt_gtf_get_type(gtf, type);
  gt_gtf_entry* e = gt_gtf_entry_new(start, end, strand, tp);
  if(gene_id != NULL){
    // get the gene_id or create it
    gt_string* gids= gt_gtf_get_gene_id(gtf, gene_id);
    e->gene_id = gids;
  }
  if(gene_type != NULL){
    // get the gene_id or create it
    gt_string* gt= gt_gtf_get_gene_type(gtf, gene_type);
    e->gene_type = gt;
  }
  if(transcript_id != NULL){
    // get the gene_id or create it
    gt_string* gt= gt_gtf_get_transcript_id(gtf, transcript_id);
    e->transcript_id = gt;
  }

  // get the ref or create it
  gt_gtf_ref* gtref = gt_gtf_get_ref(gtf, ref);
  e->uid = counter;
  gt_vector_insert(gtref->entries, e, gt_gtf_entry*);
}

bool gt_gtf_hits_junction(gt_map* map, gt_gtf_entry* e){
  uint64_t rs = gt_map_get_begin_mapping_position(map);
  uint64_t re = gt_map_get_end_mapping_position(map);
  bool hit = (rs==e->start) || (rs==e->end) || (re == e->end) || (re == e->start);
  return hit;
}

void gt_gtf_print_entry_(gt_gtf_entry* e, gt_map* map){
  if(map != NULL){
    gt_output_map_fprint_map(stdout, map, NULL);
    printf(" ==> ");
  }
  printf("%s : %"PRIu64" - %"PRIu64" (%d)", e->type->buffer, e->start, e->end, e->strand);
  if(e->gene_id != NULL){
    printf(" GID:%s", e->gene_id->buffer);
  }
  if(e->transcript_id != NULL){
    printf(" TID:%s", e->transcript_id->buffer);
  }

  if(e->type != NULL){
    printf(" [%s]", e->type->buffer);
  }
  if(map != NULL && gt_gtf_hits_junction(map, e)){
    printf(" [Hits JS]");
  }
  printf("\n");
}



GT_INLINE gt_gtf_hit* gt_gtf_hit_new(void){
  gt_gtf_hit* hit = malloc(sizeof(gt_gtf_hit));
  hit->exon_overlap = 0.0;
  hit->intron_length = 0.0;
  hit->is_protein_coding = false;
  hit->junction_hits = 0.0;
  hit->map = NULL;
  hit->num_junctions = 0;
  hit->pairs_transcript = false;
  hit->pairs_splits = false;
  hit->pairs_gene = false;
  hit->num_template_blocks = 0;
  hit->transcripts = NULL;
  hit->genes = NULL;
  return hit;
}

GT_INLINE void gt_gtf_hit_delete(gt_gtf_hit* hit){
  if(hit->transcripts != NULL){
    gt_shash_delete(hit->transcripts, true);
  }
  if(hit->genes != NULL){
    gt_shash_delete(hit->genes, true);
  }
  free(hit);
}




GT_INLINE gt_status gt_gtf_get_line(gt_buffered_input_file* const buffered_map_input, gt_string* const line) {
  GT_BUFFERED_INPUT_FILE_CHECK(buffered_map_input);
  GT_STRING_CHECK(line);

  gt_status error_code;
  // Check the end_of_block. Reload buffer if needed
  if (gt_buffered_input_file_eob(buffered_map_input)) {
    if ((error_code=gt_input_map_parser_reload_buffer(buffered_map_input,false,100))!=GT_IMP_OK) return error_code;
  }
  // Prepare the template
  char* const line_start = buffered_map_input->cursor;
  gt_string_clear(line);
  GT_INPUT_FILE_SKIP_LINE(buffered_map_input);
  gt_string_set_nstring_static(line, line_start, (buffered_map_input->cursor - line_start)-1);
  return GT_IMP_OK;
}

GT_INLINE uint64_t gt_gtf_merge_(const gt_gtf* const target, gt_gtf* source, uint64_t counter){
  // get the type or create it
  GT_SHASH_BEGIN_KEY_ITERATE(source->refs, key){
    gt_gtf_ref* source_ref = gt_gtf_get_ref(source, key);
    gt_gtf_ref* target_ref = gt_gtf_get_ref(target, key);
    GT_VECTOR_ITERATE(source_ref->entries, value, c, gt_gtf_entry*){
      gt_gtf_entry* e = *value;
      e->uid = counter++;
      if(e->gene_id != NULL){
        e->gene_id = gt_gtf_get_gene_id(target, gt_string_get_string(e->gene_id));
      }
      if(e->transcript_id != NULL){
        e->transcript_id = gt_gtf_get_transcript_id(target, gt_string_get_string(e->transcript_id));
      }
      if(e->type != NULL)e->type = gt_gtf_get_type(target, gt_string_get_string(e->type));
      if(e->gene_type != NULL)e->gene_type = gt_gtf_get_gene_type(target, gt_string_get_string(e->gene_type));
      gt_vector_insert(target_ref->entries, e, gt_gtf_entry*);
    }
  }GT_SHASH_END_ITERATE;
  return counter;
}


GT_INLINE gt_gtf* gt_gtf_read_from_stream(FILE* input, uint64_t threads){
  gt_input_file* input_file = gt_input_stream_open(input);
  return gt_gtf_read(input_file, threads);
}
GT_INLINE gt_gtf* gt_gtf_read_from_file(char* input, uint64_t threads){
  gt_input_file* input_file = gt_input_file_open(input, false);
  return gt_gtf_read(input_file, threads);
}

GT_INLINE gt_gtf* gt_gtf_read(gt_input_file* input_file, const uint64_t threads){
  uint64_t counter = 0;
  uint64_t i = 0;
  gt_gtf** gtfs = gt_calloc(threads, gt_gtf*, true);
  for(i=0; i<threads; i++){
    gtfs[i] = gt_gtf_new();
  }

  #pragma omp parallel num_threads(threads)
  {
    uint64_t tid = omp_get_thread_num();
    gt_buffered_input_file* buffered_input = gt_buffered_input_file_new(input_file);
    gt_string* buffered_line = gt_string_new(GTF_MAX_LINE_LENGTH);
    gt_gtf* thread_gtf = gtfs[tid];
    while(gt_gtf_get_line(buffered_input, buffered_line) == GT_IMP_OK){
      gt_gtf_read_line(buffered_line->buffer, thread_gtf, buffered_input->current_line_num);
      counter++;
    }
    gt_buffered_input_file_close(buffered_input);
    gt_string_delete(buffered_line);
  }
  gt_input_file_close(input_file);

  counter = 0;
  gt_gtf* gtf = gt_gtf_new();
  // merge all the thread gtfs into a single one
  for(i=0; i<threads; i++){
    counter = gt_gtf_merge_(gtf, gtfs[i], counter);
    gt_gtf_delete(gtfs[i]);
  }
  free(gtfs);


  gt_string* const exon_t = gt_string_set_new("exon");
  gt_string* const intron_t = gt_string_set_new("intron");
  // sort the refs
  GT_SHASH_BEGIN_ELEMENT_ITERATE(gtf->refs,shash_element,gt_gtf_ref) {
    // sort by start position
    gt_gtf_sort_by_start(shash_element->entries);
    uint64_t size = gt_vector_get_used(shash_element->entries);
    uint64_t i = 0;
    gt_shash* last_exons = gt_shash_new();

    for(i=0; i<size; i++){
      gt_gtf_entry* entry = *gt_vector_get_elm(shash_element->entries, i, gt_gtf_entry*);
      if(entry->type != NULL && gt_string_equals(exon_t, entry->type)){
        gt_string* transcript_id = entry->transcript_id;
        if(transcript_id != NULL){
          if(!gt_shash_is_contained(last_exons, gt_string_get_string(transcript_id))){
            gt_shash_insert(last_exons, gt_string_get_string(transcript_id), entry, gt_gtf_entry*);
          }else{
            gt_gtf_entry* prev_exon = gt_shash_get_element(last_exons, gt_string_get_string(transcript_id));
            gt_gtf_entry* intron = gt_gtf_entry_new(prev_exon->end+1,
                                                    entry->start-1,
                                                    prev_exon->strand,
                                                    intron_t);
            intron->transcript_id = transcript_id;
            intron->gene_id = prev_exon->gene_id;
            intron->uid = counter++;
            gt_vector_insert(shash_element->entries, intron, gt_gtf_entry*);
            gt_shash_remove(last_exons, gt_string_get_string(transcript_id),false);
            gt_shash_insert(last_exons, gt_string_get_string(transcript_id), entry, gt_gtf_entry*);
//            gt_gtf_print_entry_(prev_exon);
//            gt_gtf_print_entry_(intron);
//            gt_gtf_print_entry_(entry);
          }
        }

      }
    }
    gt_shash_delete(last_exons, false);


    // create a interval tree node for each ref
    shash_element->node = gt_gtf_create_node(shash_element->entries);
  } GT_SHASH_END_ITERATE
  return gtf;
}

/*
 * Binary search for start position
 */
GT_INLINE uint64_t gt_gtf_bin_search(gt_vector* const entries, const uint64_t t, const uint64_t end){
  uint64_t used = gt_vector_get_used(entries);
  uint64_t l = 0;
  uint64_t h = used - 1;
  uint64_t m = 0;

  register gt_gtf_entry* e =  *gt_vector_get_elm(entries, h, gt_gtf_entry*);
  while(l < h ){
    m = (l + h) / 2;
    e = *gt_vector_get_elm(entries, m, gt_gtf_entry*);
    if(e->start < t){
      l = m + 1;
    }else{
      h = m;
    }
  }
  e = *gt_vector_get_elm(entries, l, gt_gtf_entry*);

  if (h == l){
    return l;
  }else{
    return m;
  }
}

GT_INLINE void gt_gtf_search_node_(gt_gtf_node* node, const uint64_t start, const uint64_t end, gt_vector* const target){
  if(node == NULL) return;

  // add overlapping intervals from this node
  GT_VECTOR_ITERATE(node->entries_by_start, element, counter, gt_gtf_entry*){
    if((*element)->start > end){
      break;
    }
    gt_gtf_entry* e = *element;
    //if((*element)->start <= start && (*element)->end >= end){
    if((start < e->end && end > e->start)
      || (start >= e->start && end <=e->end)
      || (start < e->end && end >= e->end)
      || (start < e->start && end > e->end)){
      gt_vector_insert(target, (*element), gt_gtf_entry*);
    }
  }


  if(end < node->midpoint || start < node->midpoint){
    // search left tree
    gt_gtf_search_node_(node->left, start, end, target);
  }
  if (start > node->midpoint || end > node->midpoint){
    gt_gtf_search_node_(node->right, start, end, target);
  }

}




GT_INLINE uint64_t gt_gtf_search(const gt_gtf* const gtf,  gt_vector* const target, char* const ref, const uint64_t start, const uint64_t end){
  gt_vector_clear(target);
  // make sure the target ref is contained
  if (! gt_shash_is_contained(gtf->refs, ref)){
    return 0;
  }
  const gt_gtf_ref* const source_ref = gt_gtf_get_ref(gtf, ref);
  gt_gtf_search_node_(source_ref->node, start, end, target);
  return gt_vector_get_used(target);
}


GT_INLINE void gt_gtf_create_hit(gt_gtf_hit* hit, gt_map* map, gt_vector* search_hits,const gt_gtf* const gtf, gt_string* exon_type, gt_string* protein_coding){
  bool collect_transcripts = true;
  GT_MAP_ITERATE(map, map_it) {
    uint64_t start = gt_map_get_begin_mapping_position(map_it);
    uint64_t end   = gt_map_get_end_mapping_position(map_it);

    // clear search hits
    // and search for this block
    gt_vector_clear(search_hits);
    gt_gtf_search(gtf, search_hits, gt_map_get_seq_name(map_it), start, end);

    float local_overlap = 0;
    if(gt_map_has_next_block(map_it)){
      hit->intron_length += gt_map_get_junction_size(map_it);
    }
    gt_shash* local_genes = gt_shash_new();

    bool hit_junction = false;
    // get all the exons with same gene id
    GT_VECTOR_ITERATE(search_hits, element, counter, gt_gtf_entry*){
      gt_gtf_entry* entry = *element;
      if(!gt_string_equals(exon_type, entry->type) || !gt_string_equals(protein_coding, entry->gene_type)){
        continue;
      }
      //gt_gtf_print_entry_(entry, map_it);
      // we hit something protein coding
      hit->is_protein_coding = true;
      // calculate the overlap
      float read_length = (end - start) + 1;
      uint64_t tran_length = (entry->end - entry->start) + 1;
      uint64_t s = entry->start < start ? start - entry->start : 0;
      uint64_t e = entry->end > end ? entry->end - end : 0;
      float over = ((tran_length - s - e) / read_length);
      if(over >= local_overlap){
        local_overlap = over;
      }
      if(hit->num_junctions > 0 && !hit_junction){
        hit->junction_hits += gt_gtf_hits_junction(map_it, entry) ? 1 : 0;
        hit_junction = true;
      }
      // fill initial transcripts
      if(entry->transcript_id != NULL){
        if(collect_transcripts){
          if(!gt_shash_is_contained(hit->transcripts, gt_string_get_string(entry->transcript_id))){
            uint64_t* v = gt_malloc_uint64();
            *v = 1;
            gt_shash_insert(hit->transcripts, gt_string_get_string(entry->transcript_id), v, uint64_t);
          }else{
            uint64_t* v = gt_shash_get(hit->transcripts,gt_string_get_string(entry->transcript_id),uint64_t);
            ++(*v);
          }
        }else{
          if(gt_shash_is_contained(hit->transcripts, gt_string_get_string(entry->transcript_id))){
            uint64_t* v = gt_shash_get(hit->transcripts,gt_string_get_string(entry->transcript_id),uint64_t);
            ++(*v);
          }
        }
      }
      // fill initial transcripts
      if(entry->gene_id != NULL){
        // count if the gene is not in the local gene list and we counted it
        // for this hit set already
        char* gene_id = gt_string_get_string(entry->gene_id);
        if(!gt_shash_is_contained(local_genes, gene_id)){
          // insert into local list and count
          gt_shash_insert(local_genes, gene_id, true, bool);
          if(!gt_shash_is_contained(hit->genes, gene_id)){
            uint64_t* v = gt_malloc_uint64();
            *v = 1;
            gt_shash_insert(hit->genes, gene_id, v, uint64_t);
          }else{
            uint64_t* v = gt_shash_get(hit->genes,gene_id,uint64_t);
            ++(*v);
          }
        }
      }
    }
    hit->exon_overlap += local_overlap;
    gt_shash_delete(local_genes, false);
  }


}


GT_INLINE void gt_gtf_search_template_for_exons(const gt_gtf* const gtf, gt_gtf_hits* const hits, gt_template* const template_src){
  if(!gt_gtf_contains_type(gtf, "exon")){
    return;
  }
  gt_string* const exon_type = gt_gtf_get_type(gtf, "exon");
  gt_string* const protein_coding = gt_string_set_new("protein_coding");
  // stores hits
  gt_vector* const search_hits = gt_vector_new(32, sizeof(gt_gtf_entry*));
  // clear the hits
  gt_gtf_hits_clear(hits);

  // process single alignments
  GT_TEMPLATE_IF_REDUCES_TO_ALINGMENT(template_src,alignment_src) {
    GT_ALIGNMENT_ITERATE(alignment_src,map) {
      gt_gtf_hit* hit = gt_gtf_hit_new();
      hit->map = map;
      hit->num_junctions = gt_map_get_num_blocks(map) - 1;
      hit->transcripts = gt_shash_new();
      hit->genes = gt_shash_new();

      gt_gtf_create_hit(hit, map, search_hits, gtf, exon_type, protein_coding);


      hit->exon_overlap = hit->exon_overlap / gt_map_get_num_blocks(map);
      if(hit->num_junctions > 0){
        hit->junction_hits = hit->junction_hits / hit->num_junctions;
      }

      GT_SHASH_BEGIN_ELEMENT_ITERATE(hit->transcripts, count, uint64_t){
        if((*count) > 1 && (*count) == gt_map_get_num_blocks(map)){
          hit->pairs_splits = true;
        }
      }GT_SHASH_END_ITERATE;
      if(gt_map_get_num_blocks(map) >0){
        GT_SHASH_BEGIN_ELEMENT_ITERATE(hit->genes, count, uint64_t){
          if((*count) == gt_map_get_num_blocks(map)){
            hit->pairs_gene = true;
          }
        }GT_SHASH_END_ITERATE;
      }else{
        // always mark as pairing if we dont know better
        hit->pairs_gene = true;
      }


      gt_vector_insert(hits->exon_hits, hit, gt_gtf_hit*);
    }
    gt_string_delete(protein_coding);
    gt_vector_delete(search_hits);
  } GT_TEMPLATE_END_REDUCTION__RETURN;


  GT_TEMPLATE_ITERATE_MMAP__ATTR(template_src,mmap,mmap_attr) {
    gt_gtf_hit* template_hit = gt_gtf_hit_new();
    template_hit->mmap = mmap;
    template_hit->map_attributes = mmap_attr;
    template_hit->num_template_blocks = gt_template_get_num_blocks(template_src);
    GT_MMAP_ITERATE(mmap, map, end_position){
      gt_gtf_hit* hit = NULL;
      if(end_position == 0){
        hit = template_hit;
      }else{
        hit = gt_gtf_hit_new();
      }

      hit->map = map;
      hit->num_junctions = gt_map_get_num_blocks(map) - 1;
      hit->transcripts = gt_shash_new();
      hit->genes = gt_shash_new();

      gt_gtf_create_hit(hit, map, search_hits, gtf, exon_type, protein_coding);


      hit->exon_overlap = hit->exon_overlap / gt_map_get_num_blocks(map);
      if(hit->num_junctions > 0){
        hit->junction_hits = hit->junction_hits / (hit->num_junctions*2.0);
      }
      GT_SHASH_BEGIN_ELEMENT_ITERATE(hit->transcripts, count, uint64_t){
        if((*count) > 1 && (*count) == gt_map_get_num_blocks(map)){
          hit->pairs_splits = true;
        }
      }GT_SHASH_END_ITERATE;

      if(end_position > 0){
        // merge and remove
        template_hit->exon_overlap = (template_hit->exon_overlap+hit->exon_overlap)/2.0;
        if(template_hit->num_junctions > 0 && hit->num_junctions > 0){
          template_hit->pairs_splits = template_hit->pairs_splits & hit->pairs_splits;
        }else{
          template_hit->pairs_splits = template_hit->num_junctions > 0 ? template_hit->pairs_splits : hit->pairs_splits;
        }


        template_hit->is_protein_coding = template_hit->is_protein_coding & hit->is_protein_coding;
        template_hit->intron_length += hit->intron_length;
        template_hit->num_junctions += hit->num_junctions;


        GT_SHASH_BEGIN_KEY_ITERATE(hit->transcripts, key){
          if(gt_shash_is_contained(template_hit->transcripts, key)){
            uint64_t* v = gt_shash_get(hit->transcripts, key, uint64_t);
            uint64_t* u = gt_shash_get(template_hit->transcripts, key, uint64_t);
            *u += (*v);
          }else{
            // remove
            gt_shash_remove(template_hit->transcripts, key, true);
          }

        }GT_SHASH_END_ITERATE;
        GT_SHASH_BEGIN_KEY_ITERATE(hit->genes, key){
          if(gt_shash_is_contained(template_hit->genes, key)){
            uint64_t* v = gt_shash_get(hit->genes, key, uint64_t);
            uint64_t* u = gt_shash_get(template_hit->genes, key, uint64_t);
            *u += (*v);
          }else{
            // remove
            gt_shash_remove(template_hit->genes, key, true);
          }
        }GT_SHASH_END_ITERATE;

        template_hit->junction_hits = (template_hit->junction_hits + hit->junction_hits) / 2.0;
        GT_SHASH_BEGIN_ELEMENT_ITERATE(template_hit->transcripts, count, uint64_t){
          if((*count) > 1 && (*count) == gt_map_get_num_blocks(mmap[1]) + gt_map_get_num_blocks(mmap[0])){
            hit->pairs_transcript = true;
          }
        }GT_SHASH_END_ITERATE;

        GT_SHASH_BEGIN_ELEMENT_ITERATE(template_hit->genes, count, uint64_t){
          if((*count) > 1 && (*count) == gt_map_get_num_blocks(mmap[1]) + gt_map_get_num_blocks(mmap[0])){
            template_hit->pairs_gene = true;
          }
        }GT_SHASH_END_ITERATE;

        gt_vector_insert(hits->exon_hits, template_hit, gt_gtf_hit*);
        gt_gtf_hit_delete(hit);
      }
    }
  }
  gt_string_delete(protein_coding);
  gt_vector_delete(search_hits);
}

GT_INLINE void gt_gtf_count_(gt_shash* const table, char* const element){
  if(!gt_shash_is_contained(table, element)){
    uint64_t* v = gt_malloc_uint64();
    *v = 1;
    gt_shash_insert(table, element, v, uint64_t);
  }else{
    uint64_t* v = gt_shash_get(table,element,uint64_t);
    ++(*v);
  }
}

GT_INLINE uint64_t gt_gtf_get_count_(gt_shash* const table, char* const element){
  if(!gt_shash_is_contained(table, element)){
    return 0;
  }
  uint64_t* v = gt_shash_get(table,element,uint64_t);
  return *v;
}

GT_INLINE void gt_gtf_count_add_(gt_shash* const source, gt_shash* const target){
  GT_SHASH_BEGIN_ITERATE(source, key, value, uint64_t){
    if(!gt_shash_is_contained(target, key)){
      uint64_t* v = gt_malloc_uint64();
      *v = *value;
      gt_shash_insert(target, key, v, uint64_t);
    }else{
      uint64_t* v = gt_shash_get(target,key,uint64_t);
      *v += (*value);
    }
  }GT_SHASH_END_ITERATE;
}


GT_INLINE void gt_gtf_count_map_(gt_gtf* const gtf, gt_map* const map, gt_shash* const type_counts, gt_shash* const gene_counts){
  uint64_t start = gt_map_get_begin_mapping_position(map);
  uint64_t end   = gt_map_get_end_mapping_position(map);
  gt_vector* const hits = gt_vector_new(32, sizeof(gt_gtf_entry*));
  gt_gtf_search(gtf, hits, gt_map_get_seq_name(map), start, end);

  gt_shash* const local_type_counts = gt_shash_new();
  gt_shash* const local_gene_counts = gt_shash_new();

  GT_VECTOR_ITERATE(hits, e, i, gt_gtf_entry*){
    gt_gtf_entry* hit = *e;
    gt_gtf_count_(local_type_counts, gt_string_get_string(hit->type));
    gt_gtf_count_(local_gene_counts, gt_string_get_string(hit->gene_id));
  }

  // count types
  if(gt_gtf_get_count_(local_type_counts, "exon") > 0){
    gt_gtf_count_(type_counts, "exon");
  }else if(gt_gtf_get_count_(local_type_counts, "intron") > 0){
    gt_gtf_count_(type_counts, "intron");
  }else{
    gt_gtf_count_(type_counts, "unknown");
  }

  if(gt_shash_get_num_elements(local_gene_counts) == 1){
    // add gene count
    GT_SHASH_BEGIN_KEY_ITERATE(local_gene_counts, key){
      gt_gtf_count_(gene_counts, key);
      break;
    }GT_SHASH_END_ITERATE;
  }

  gt_shash_delete(local_gene_counts, true);
  gt_shash_delete(local_type_counts, true);
  gt_vector_delete(hits);
}

GT_INLINE void gt_gtf_count_map(gt_gtf* const gtf, gt_map* const map, gt_shash* const type_counts, gt_shash* const gene_counts){
  uint64_t blocks = gt_map_get_num_blocks(map);
  if(blocks == 1){
    // single block
    gt_gtf_count_map_(gtf, map, type_counts, gene_counts);
  }else{
    // splitmap with multiple blocks
    gt_shash* const local_type_counts = gt_shash_new();
    gt_shash* const local_gene_counts = gt_shash_new();
    GT_MAP_ITERATE(map, map_block){
      gt_gtf_count_map_(gtf, map_block, local_type_counts, local_gene_counts);
    }
    // count type
    // count types
    uint64_t exons = gt_gtf_get_count_(local_type_counts, "exon");
    uint64_t introns = gt_gtf_get_count_(local_type_counts, "intron");
    uint64_t unknown = gt_gtf_get_count_(local_type_counts, "unknown");
    if(exons == blocks){
      gt_gtf_count_(type_counts, "exon");
    }else if(introns == blocks){
      gt_gtf_count_(type_counts, "intron");
    }else if(unknown == blocks){
      gt_gtf_count_(type_counts, "unknown");
    }else{
      // construct type
      gt_string* t = gt_string_new(16);
      uint64_t total = exons + introns + unknown;

      if(exons > 0){
        gt_string_right_append_string(t, "exon", 4);
        total -= exons;
        if(total > 0) gt_string_append_char(t, '|');
      }

      if(introns > 0){
        gt_string_right_append_string(t, "intron", 6);
        total -= introns;
        if(total > 0) gt_string_append_char(t, '|');
      }

      if(unknown > 0){
        gt_string_right_append_string(t, "unknown", 7);
        total -= unknown;
      }
      gt_gtf_count_(type_counts, gt_string_get_string(t));
      gt_string_delete(t);
    }

    // count genes
    // only count if the split is within a single gene
    if(gt_shash_get_num_elements(local_gene_counts) == blocks){
      // add gene count
      GT_SHASH_BEGIN_KEY_ITERATE(local_gene_counts, key){
        gt_gtf_count_(gene_counts, key);
        break;
      }GT_SHASH_END_ITERATE;
    }
    gt_shash_delete(local_gene_counts, true);
    gt_shash_delete(local_type_counts, true);
  }
}

GT_INLINE void gt_gtf_count_alignment(gt_gtf* const gtf, gt_alignment* const alignment, gt_shash* const type_count, gt_shash* const gene_counts){
  if(gt_alignment_get_num_maps(alignment) != 1) return; // count only unique
  GT_ALIGNMENT_ITERATE(alignment,map) {
    gt_gtf_count_map(gtf, map,type_count, gene_counts);
  }
}

GT_INLINE void gt_gtf_count_template(gt_gtf* const gtf, gt_template* const template, gt_shash* const type_counts, gt_shash* const gene_counts){
  // process single alignments
  GT_TEMPLATE_IF_REDUCES_TO_ALINGMENT(template,alignment) {
      gt_gtf_count_alignment(gtf, alignment, type_counts, gene_counts);
  } GT_TEMPLATE_END_REDUCTION__RETURN;

  // process templates
  GT_TEMPLATE_ITERATE_MMAP__ATTR_(template,mmap,mmap_attr) {
    if(gt_template_get_num_mmaps(template) != 1) continue; // count only unique
    gt_gtf_count_map(gtf, mmap[0],type_counts, gene_counts);
    gt_gtf_count_map(gtf, mmap[1],type_counts, gene_counts);
  }
}
