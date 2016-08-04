/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/plat/acpi/acpi.h>
#include "acpi.h"

#include "walker.h"

#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _GNU_SOURCE /* for getpagesize() */
#include <unistd.h>

#define SIG_SEARCH_STEP (16) /* value from ACPIAC RSDP search */

static void*
_sig_search(const char* sig, int sig_len, const char* loc, const char* end)
{
    for (; loc < end; loc += SIG_SEARCH_STEP) {
        if (strncmp(sig, loc, sig_len) == 0) {
            return (void*)loc;
        }
    }
    return NULL;
}

static void *
acpi_map_table(acpi_t *acpi, void *table_paddr)
{

    /* map the first part of the page in to read the size */
    acpi_header_t *header = (acpi_header_t *) ps_io_map(&acpi->io_mapper,
                                                        (uintptr_t)table_paddr, sizeof(acpi_header_t), 1, PS_MEM_NORMAL);

    if (header == NULL) {
        fprintf(stderr, "Failed to map paddr %p, size %u\n", table_paddr, sizeof(acpi_header_t));
        assert(header != NULL);
        return NULL;
    }

    uint32_t length = acpi_table_length(header);
    if (length == 0xffffffff) {
        fprintf(stderr, "Skipping table %s, unknown\n", header->signature);
        return NULL;
    }

    /* if the size is bigger than a page, unmap and remap a contiguous region */
    if (!SAME_PAGE_4K(header, ((void *) header) + length)) {
        ps_io_unmap(&acpi->io_mapper, (void *) header, sizeof(acpi_header_t));
        header = ps_io_map(&acpi->io_mapper, (uintptr_t)table_paddr, length, 1, PS_MEM_NORMAL);

        if (header == NULL) {
            fprintf(stderr, "Failed tomap paddr %p, size %u\n", table_paddr, header->length);
            assert(header != NULL);
            return NULL;
        }
    }

    return header;
}

static void
acpi_unmap_table(acpi_t *acpi, acpi_header_t *header)
{
    ps_io_unmap(&acpi->io_mapper, (void *) header, acpi_table_length(header));
}

void*
acpi_sig_search(acpi_t *acpi, const char* sig, int sig_len, void* start, void* end)
{
    void *found = NULL;
    void *vaddr;

    /* work a page at a time, searching for the target string */
    while (start < end && !found) {
        vaddr = ps_io_map(&acpi->io_mapper, (uintptr_t) start, getpagesize(), 1, PS_MEM_NORMAL);
        if (vaddr == NULL) {
            fprintf(stderr, "Failed to map physical page %p\n", start);
            return NULL;
        }

        found = _sig_search(sig, sig_len, vaddr, vaddr + getpagesize());

        if (!found) {
            start += getpagesize();
        }

        ps_io_unmap(&acpi->io_mapper, vaddr, getpagesize());
    }

    if (!found) {
        fprintf(stderr, "Faied to find sig %s in range %p <-> %p\n", sig, start, end);
        return NULL;
    }

    /* return the physical address of sig */
    return (void *) start + ((uint32_t)found % getpagesize());
}

static acpi_header_t*
acpi_parse_table(acpi_t *acpi, void *table_paddr)
{

    /* map the table into virtual memory */
    acpi_header_t* header_vaddr = acpi_map_table(acpi, table_paddr);
    if (header_vaddr == NULL) {
        return NULL;
    }

    /* now create a copy of the table for us to keep */
    uint32_t length = acpi_table_length(header_vaddr);
    acpi_header_t *copy = (acpi_header_t *) malloc(length);
    if (copy == NULL) {
        fprintf(stderr, "Failed to malloc object size %u\n", length);
        assert(copy != NULL);
        return NULL;
    }

    memcpy(copy, header_vaddr, length);

    /* finally, unmap the original table */
    acpi_unmap_table(acpi, header_vaddr);

    /* return the copy.
     *
     * The reason we do this in this round-about way is:
    *
     * Acpi tables can be scattered all over the entire memory range.
     * We can't guarantee that we can map in all the tables at their addresses
     * as that address may not be available.
     *
     * But we can be confident that the acpi tables don't take up all of memory.
     * By copying them to dynamic memory, we can keep them all in one place.
     */
    return copy;
}

static void
_acpi_parse_tables(acpi_t *acpi, void* table_paddr, RegionList_t* regions,
                   int parent)
{

    int this_rec;
    region_type_t type;

    if (table_paddr == NULL) {
        return;
    }

    acpi_header_t* header = acpi_parse_table(acpi, table_paddr);
    if (header == NULL) {
        /* skip table */
        return;
    }

    void *table_vaddr = (void *) header;


    type = acpi_sig_id(header->signature);

    // optimistic: remove later if the table is bad
    this_rec = add_region_size(regions, type, table_vaddr,
                               header->length, parent);
    if (this_rec < 0) {
        return;    /* List full */
    }

    switch (type) {
        /*******************************************
         * These tables are completely implemented *
         *******************************************/
    case ACPI_RSDP: {
        acpi_rsdp_t* rsdp = (acpi_rsdp_t*) table_vaddr;
        /*
         * This table does not have a standard acpi header.
         * Adjust for this earlier assumption
         */
        regions->regions[this_rec].size = rsdp->length;

        /* parse sub tables */
        _acpi_parse_tables(acpi, (void*)rsdp->rsdt_address,
                           regions, this_rec);
        _acpi_parse_tables(acpi, (void*)(uint32_t)rsdp->xsdt_address,
                           regions, this_rec);
        break;
    }
    case ACPI_RSDT: {
        acpi_rsdt_t* rsdt = (acpi_rsdt_t*) table_vaddr;
        uint32_t* subtbl = acpi_rsdt_first(rsdt);
        while (subtbl != NULL) {
            _acpi_parse_tables(acpi, (void*)*subtbl,
                               regions, this_rec);
            subtbl = acpi_rsdt_next(rsdt, subtbl);
        }
        break;
    }

    /*
     * XSDT is the same as RSDT but with 64bit addresses
     * Don't parse this table to avoid duplicate entries
     *
     * TODO this could actually contain unique entries,
     * need to parse and sort out dups.
     */
    case ACPI_XSDT: {
        fprintf(stderr, "Warning: skipping table ACPI XSDT\n");
//            acpi_xsdt_t* xsdt = (acpi_xsdt_t*)table;
        break;
    }

    case ACPI_FADT: {
        acpi_fadt_t* fadt = (acpi_fadt_t*)table_vaddr;
        _acpi_parse_tables(acpi, (void*)fadt->facs_address,
                           regions, this_rec);
        _acpi_parse_tables(acpi, (void*)fadt->dsdt_address,
                           regions, this_rec);
        break;
    }

    /******************************************
     * These tables use a standard header and *
     * have no sub-tables                     *
     ******************************************/
    case ACPI_HPET:
    case ACPI_BOOT:
    case ACPI_SPCR:
    case ACPI_MCFG:
    case ACPI_SPMI:
    case ACPI_SSDT:
    case ACPI_DSDT:
    case ACPI_FACS:
    case ACPI_MADT:
    case ACPI_ERST: {
        break;
    }

    /*********************************************
     * These tables use a standard header and    *
     * have no sub-tables but depend on device   *
     * caps that may not be available. It may be *
     * best to withhold these tables from linux  *
     *********************************************/
    case ACPI_ASF :
    case ACPI_DMAR: {
        break;
    }

    /******************************************
     * These tables are partially implemented *
     ******************************************/
    case ACPI_BERT: {
//            acpi_bert_t* bert = (acpi_bert_t*)table;
        /* not complemetely implemented so exclude */
        fprintf(stderr, "Warning: skipping table ACPI_BERT (unimplemented)");
        remove_region(regions, this_rec);
        break;
    }
    case ACPI_EINJ: {
//            acpi_einj_t* einj = (acpi_einj_t*)table;
        /* not complemetely implemented so exclude */
        fprintf(stderr, "Warning: skipping table ACPI_EINJ (unimplemented)");
        remove_region(regions, this_rec);
        break;
    }
    case ACPI_HEST: {
//            acpi_hest_t* hest = (acpi_hest_t*)table;
        /* not complemetely implemented so exclude */
        fprintf(stderr, "Warning: skipping table ACPI_HEST (unimplemented)");
        remove_region(regions, this_rec);
        break;
    }

    /*******************************************
     * These tables are not implemented at all *
     *******************************************/
    case ACPI_ASPT:/* present on Dogfood machine: unknown table */
    case ACPI_MSCT:
    case ACPI_CPEP:
    case ACPI_ECDT:
    case ACPI_SBST:
    case ACPI_SLIT:
    case ACPI_SRAT:
        /* Not implemented */
        fprintf(stderr, "Warning: skipping table %s (unimplemented)", header->signature);
        remove_region(regions, this_rec);
        break;

    default:
        fprintf(stderr, "Warning: skipping table %s (unimplemented)", header->signature);
        remove_region(regions, this_rec);
    }
    return;
}

void
acpi_parse_tables(acpi_t* acpi)
{

    RegionList_t* regions = (RegionList_t *) acpi->regions;
    regions->region_count = 0;
    regions->offset = 0;

    _acpi_parse_tables(acpi, acpi->rsdp, regions, -1);
}
