// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_h5z_zfp_smoke.cpp
///
//-----------------------------------------------------------------------------
#include <hdf5.h>
#include "H5Zzfp_lib.h"
#include "H5Zzfp_props.h"

#include <iostream>
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(h5z_zfp_smoke, basic_use)
{
    /* identifiers  */
    hid_t     file_id;
    hid_t     dataset_cprops_id;
    hid_t     dataset_id;
    hid_t     dataspace_id;
    hsize_t   dims[2];
    hsize_t   chunks[2];
    herr_t    status;


    chunks[0] = 256;
    chunks[1] = 8;

    // example 2d array
    dims[0] = chunks[0] * 2;
    dims[1] = chunks[1] * 3;

    hsize_t ntotal = dims[0] * dims[1];

    double * vals = new double[ntotal];

    for (hsize_t i = 0; i < ntotal; i++)
    {
        vals[i] = i;
    }

    /* Create a new file using default properties. */
    file_id = H5Fcreate("h5zzfp_smoke_test.hdf5",
                        H5F_ACC_TRUNC,
                        H5P_DEFAULT,
                        H5P_DEFAULT);

    EXPECT_TRUE (file_id >= 0 );

    /* Create the data space for the dataset. */
    dataspace_id = H5Screate_simple(2, dims, NULL);
    EXPECT_TRUE (dataspace_id >= 0 );

    /* setup dataset creation properties */
    dataset_cprops_id = H5Pcreate(H5P_DATASET_CREATE);
    EXPECT_TRUE(dataset_cprops_id >= 0);

    status = H5Pset_chunk(dataset_cprops_id, 2, chunks);
    EXPECT_TRUE(status >= 0);

    /* When the h5z-zfp filter is used as a library, we need to init it */
    H5Z_zfp_initialize();

    /* use zfp rate mode */
    H5Pset_zfp_rate(dataset_cprops_id, 4);

    /* Create the dataset. */
    dataset_id = H5Dcreate(file_id,
                           "/compressed_dset",
                           H5T_NATIVE_DOUBLE,
                           dataspace_id,
                           H5P_DEFAULT,
                           dataset_cprops_id,
                           H5P_DEFAULT);

    EXPECT_TRUE (dataset_id >= 0);

    /* write our data */
    status = H5Dwrite(dataset_id,
                      H5T_NATIVE_DOUBLE,
                      H5S_ALL,
                      H5S_ALL,
                      H5P_DEFAULT,
                      vals);

    EXPECT_TRUE(status >= 0 );

    /* End access to the prop list and release resources used by it. */
    status = H5Pclose(dataset_cprops_id);
    EXPECT_TRUE(status >= 0 );

    /* End access to the dataset and release resources used by it. */
    status = H5Dclose(dataset_id);
    EXPECT_TRUE(status >= 0 );

    /* Terminate access to the data space. */ 
    status = H5Sclose(dataspace_id);
    EXPECT_TRUE(status >= 0 );

    /* Close the file. */
    status = H5Fclose(file_id);
    EXPECT_TRUE(status >= 0 );

    /* cleanup our vals array */
    delete [] vals;

    /* cleanup h5z-zfp */
    H5Z_zfp_finalize();
}
