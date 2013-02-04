#!/usr/bin/env
"""Pipeline utilities"""
import sys
import os
import logging
import gem

from gem.filter import interleave, unmapped
from gem.filter import filter as gf
from gem.utils import Timer


class MappingPipeline(object):

    def __init__(self, name=None, index=None,
        output_dir=None,
        annotation=None,
        threads=2,
        junctioncoverage=4,
        maxlength=100,
        transcript_index=None,
        transcript_keys=None,
        quality=33,
        delta=1,
        remove_temp=True
        ):
        self.remove_temp = remove_temp
        self.name = name
        self.index = index
        self.output_dir = output_dir
        self.annotation = annotation
        self.threads = threads
        self.junctioncoverage = junctioncoverage
        self.maxlength = maxlength
        self.transcript_index = transcript_index
        self.transcript_keys = transcript_keys
        self.denovo_index = None
        self.denovo_keys = None
        self.delta = delta
        self.quality = quality
        self.junctions_file = None
        self.mappings = []
        self.files = []

        if self.output_dir is None:
            self.output_dir = os.getcwd()

        ## check if output is specified and create folder in case it does not exists
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        if self.transcript_index is None:
            self.transcript_index = self.annotation + ".gem"
        if self.transcript_keys is None and self.transcript_index is not None:
            self.transcript_keys = self.transcript_index[:-4] + ".junctions.keys"

    def cleanup(self):
        """Delete all remaining temporary and intermediate files
        """
        if self.remove_temp:
            for f in self.files:
                if os.path.exists(f):
                    logging.info("Removing intermediate file : %s" % (f))
                    os.remove(f)

    def create_file_name(self, suffix, file_suffix="map", final=False):
        """Create a result file name"""
        file = ""
        if suffix is not None and len(suffix) > 0:
            file = "%s/%s_%s.%s" % (self.output_dir, self.name, suffix, file_suffix)
        else:
            file = "%s/%s.%s" % (self.output_dir, self.name, file_suffix)
        if not final:
            self.files.append(file)
        return file

    def _guess_input_name(self, input):
        try:
            return input.filename
        except Exception:
            return "STREAM"

    def merge(self, suffix):
        """Merge current set of mappings and delete last ones"""
        out = self.create_file_name(suffix)
        if os.path.exists(out):
            logging.warning("Merge target exists, skipping merge : %s" % (out))
            return gem.files.open(out, type="map", quality=self.quality)

        logging.info("Merging %d mappings into %s" % (len(self.mappings), out))
        timer = Timer()
        merged = gem.merger(
            self.mappings[0].clone(),
            [m.clone() for m in self.mappings[1:]]
        ).merge(out, self.threads)
        if self.remove_temp:
            for m in self.mappings:
                logging.info("Removing temporary mapping %s ", m.filename)
                os.remove(m.filename)
        self.mappgins = []
        self.mappings.append(merged)
        timer.stop("Merging finished in %s")
        return merged

    def mapping_step(self, input, suffix, trim=None):
        """Single mapping step where the input is passed on
        as is to the mapper. The output name is created based on
        the dataset name in the parameters the suffix.

        The function returns an opened ReadIterator on the resulting
        mapping

        input -- mapper input
        suffix -- output name suffix
        trim -- optional trimming options, i.e. '0,20'
        """
        timer = Timer()
        mapping_out = self.create_file_name(suffix)
        if os.path.exists(mapping_out):
            logging.warning("Mapping step target exists, skip mapping set : %s" % (mapping_out))
            mapping = gem.files.open(mapping_out, type="map", quality=self.quality)
            self.mappings.append(mapping)
            return mapping
        input_name = self._guess_input_name(input)
        logging.debug("Mapping from %s to %s" % (input_name, mapping_out))
        mapping = gem.mapper(input,
                self.index,
                mapping_out,
                mismatches=0.06,
                delta=self.delta,
                trim=trim,
                quality=self.quality,
                threads=self.threads)
        self.mappings.append(mapping)
        timer.stop("Mapping step finished in %s")
        return mapping

    def split_mapping_step(self, input, suffix, trim=None):
        """Single split-mapping step where the input is passed on
        as is to the mapper. The output name is created based on
        the dataset name in the parameters the suffix.

        The function returns an opened ReadIterator on the resulting
        mapping

        input -- mapper input
        suffix -- output name suffix
        trim -- optional trimming options, i.e. '0,20'
        """
        timer = Timer()
        mapping_out = self.create_file_name(suffix)
        if os.path.exists(mapping_out):
            logging.warning("Split-Mapping step target exists, skip mapping set : %s" % (mapping_out))
            mapping = gem.files.open(mapping_out, type="map", quality=self.quality)
            self.mappings.append(mapping)
            return mapping
        input_name = self._guess_input_name(input)
        logging.debug("Split-Mapping from %s to %s" % (input_name, mapping_out))

        mapping = gem.splitmapper(
            input,
            self.index,
            mapping_out,
            junctions_file=self.junctions_file,
            trim=trim,
            threads=self.threads,
            quality=self.quality,
            mismatches=0.06)
        self.mappings.append(mapping)
        timer.stop("Split-Mapping step finished in %s")
        return mapping

    def transcript_mapping_step(self, input, suffix, index=None, key=None, trim=None):
        """Single transcript mapping step where the input is passed on
        as is to the mapper. The output name is created based on
        the dataset name in the parameters the suffix.

        The function returns an opened ReadIterator on the resulting
        mapping

        input -- mapper input
        suffix -- output name suffix
        trim -- optional trimming options, i.e. '0,20'
        """
        timer = Timer()
        mapping_out = self.create_file_name(suffix + "_transcript")
        if os.path.exists(mapping_out):
            logging.warning("Transcript mapping step target exists, skip mapping set : %s" % (mapping_out))
            mapping = gem.files.open(mapping_out, type="map", quality=self.quality)
            self.mappings.append(mapping)
            return mapping
        input_name = self._guess_input_name(input)
        logging.debug("Mapping transcripts from %s to %s" % (input_name, mapping_out))

        indices = [self.transcript_index, self.denovo_index]
        keys = [self.transcript_keys, self.denovo_keys]

        if index is not None:
            indices = [index]
            if key is None:
                key = index[:-4] + ".junctions.keys"
            keys = [key]

        mapping = gem.transcript_mapper(
                                input,
                                indices,
                                keys,
                                mapping_out,
                                mismatches=0.06,
                                min_decoded_strata=0,
                                trim=trim,
                                delta=self.delta,
                                quality=self.quality,
                                threads=self.threads
                                )
        self.mappings.append(mapping)
        timer.stop("Transcript-Mapping step finished in %s")
        return mapping

    def gtf_junctions(self):
        """check if there is a .junctions file for the given annotation, if not,
        create it. Returns a tuple of the set of junctions and the output file.
        """
        timer = Timer()
        gtf_junctions = self.annotation + ".junctions"
        out = None
        junctions = None
        if os.path.exists(gtf_junctions):
            logging.info("Loading existing junctions from %s" % (gtf_junctions))
            out = gtf_junctions
            junctions = set(gem.junctions.from_junctions(gtf_junctions))
        else:
            out = self.create_file_name("gtf", file_suffix="junctions")
            if os.path.exists(out):
                logging.info("Loading existing junctions from %s" % (out))
                junctions = set(gem.junctions.from_junctions(out))
            else:
                logging.info("Extracting junctions from %s" % (self.annotation))
                junctions = set(gem.junctions.from_gtf(self.annotation))
                gem.junctions.write_junctions(junctions, out, self.index)

        logging.info("%d Junctions from GTF" % (len(junctions)))
        timer.stop("GTF-Junctions prepared in %s")
        return (junctions, out)

    def create_denovo_transcriptome(self, input):
        """Squeeze input throw the split mapper to extract denovo junctions
        and create a transcriptome out of the junctions"""
        index_denovo_out = self.create_file_name("denovo_transcripts", file_suffix="gem")
        junctions_out = self.create_file_name("all", file_suffix="junctions")
        denovo_keys = junctions_out + ".keys"
        if os.path.exists(index_denovo_out) and os.path.exists(denovo_keys):
            logging.warning("Transcriptome index and keys found, skip creating : %s" % (index_denovo_out))
            ## add .fa and .keys to list of files to delete
            self.files.append(junctions_out + ".fa")
            self.files.append(junctions_out + ".keys")
            self.files.append(index_denovo_out[:-4] + ".log")
            self.denovo_keys = denovo_keys
            self.denovo_index = index_denovo_out
            return index_denovo_out

        (junctions, junctions_gtf_out) = self.gtf_junctions()

        timer = Timer()
        ## get de-novo junctions
        logging.info("Getting de-novo junctions")

        ## add .fa and .keys to list of files to delete
        self.files.append(junctions_out + ".fa")
        self.files.append(junctions_out + ".keys")

        junctions = gem.extract_junctions(
            input,
            self.index,
            mismatches=0.04,
            threads=self.threads,
            strata_after_first=0,
            coverage=self.junctioncoverage,
            merge_with=junctions)
        logging.info("Total Junctions %d" % (len(junctions)))
        timer.stop("Denovo Transcripts extracted in %s")
        timer = Timer()
        gem.junctions.write_junctions(gem.junctions.filter_by_distance(junctions, 500000), junctions_out, self.index)

        logging.info("Computing denovo transcriptome")
        (denovo_transcriptome, denovo_keys) = gem.compute_transcriptome(self.maxlength, self.index, junctions_out, junctions_gtf_out)
        logging.info("Indexing denovo transcriptome")
        timer.stop("Transcriptome generated in %s")
        timer = Timer()
        idx = gem.index(denovo_transcriptome, index_denovo_out, threads=self.threads)
        timer.stop("Transcriptome indexed in %s")
        # add indexer log file to list of files
        self.files.append(idx[:-4] + ".log")
        self.denovo_keys = denovo_keys
        self.denovo_index = idx
        # do not add the denovo mapping but make sure file goes to temp files
        #self.mappings.append(denovo_mapping)
        #self.files.append(denovo_out)
        return idx

    def create_denovo_junctions(self, input):
        """Extract junctions from input and merge them with the gtf_junctions"""

        junctions_out = self.create_file_name("all", file_suffix="junctions")
        if os.path.exists(junctions_out):
            logging.warning("Junctions found, skip creating : %s" % (junctions_out))
            self.junctions_file = junctions_out
            return junctions_out

        timer = Timer()
        (junctions, junctions_gtf_out) = self.gtf_junctions()
        ## get de-novo junctions
        logging.info("Getting de-novo junctions")

        junctions = gem.extract_junctions(
            input,
            self.index,
            mismatches=0.04,
            threads=self.threads,
            strata_after_first=0,
            coverage=self.junctioncoverage,
            merge_with=junctions)
        logging.info("Total Junctions %d" % (len(junctions)))
        timer.stop("Junctions extracted in %s")
        timer = Timer()
        gem.junctions.write_junctions(gem.junctions.filter_by_distance(junctions, 500000), junctions_out, self.index)
        self.junctions_file = junctions_out
        return junctions_out

    def pair_align(self, input, compress=False):
        logging.info("Running pair aligner")
        timer = Timer()
        paired_out = self.create_file_name("", final=True)
        out_name = paired_out
        if compress:
            out_name = out_name + ".gz"
        if os.path.exists(out_name):
            logging.warning("Pair-alignment exists, skip creating : %s" % (paired_out))
            return gem.files.open(out_name, type="map", quality=self.quality)

        paired_mapping = gem.pairalign(input, self.index, None, max_insert_size=100000, threads=self.threads, quality=self.quality)
        scored = gem.score(paired_mapping, self.index, paired_out, threads=self.threads)
        timer.stop("Pair-Align and scoring finished in %s")
        if compress:
            logging.info("Compressing final mapping")
            timer = Timer()
            gem.utils.gzip(paired_out, threads=self.threads)
            timer.stop("Results compressed in %s")
        return scored

    def create_bam(self, input, sort=True):
        logging.info("Converting to sam/bam")
        sam_out = self.create_file_name("", file_suffix="bam", final=True)
        if os.path.exists(sam_out):
            logging.warning("SAM/BAM exists, skip creating : %s" % (sam_out))
        else:
            timer = Timer()
            sam = gem.gem2sam(input, self.index, threads=max(1, int(self.threads / 2)))
            gem.sam2bam(sam, sam_out, sorted=sort)
            timer.stop("BAM file created in %s")