// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_docs_relay_io_hdf5_examples.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_relay.hpp"
#include "conduit_relay_io_hdf5.hpp"
#include "t_conduit_docs_tutorial_helpers.hpp"

#include <iostream>
#include "gtest/gtest.h"
using namespace conduit;

//-----------------------------------------------------------------------------
TEST(conduit_docs, relay_io_example_hdf5_interface_1)
{
    BEGIN_EXAMPLE("relay_io_example_hdf5_interface_1");
    // setup node to save
    Node n;
    n["a/my_data"] = 1.0;
    n["a/b/my_string"] = "value";
    std::cout << "\nNode to write:" << std::endl;
    n.print();
    
    // open hdf5 file and obtain a handle
    hid_t h5_id = conduit::relay::io::hdf5_create_file("myoutput.hdf5");
    
    // write data 
    conduit::relay::io::hdf5_write(n,h5_id);

    // close our file
    conduit::relay::io::hdf5_close_file(h5_id);
        
    // open our file to read
    h5_id = conduit::relay::io::hdf5_open_file_for_read_write("myoutput.hdf5");

    // check if a subpath exists
    if(conduit::relay::io::hdf5_has_path(h5_id,"a/my_data"))
        std::cout << "\nPath 'myoutput.hdf5:a/my_data' exists" << std::endl;
        
    Node n_read;
    // read a subpath (Note: read works like `load_merged`)
    conduit::relay::io::hdf5_read(h5_id,"a/my_data",n_read);
    std::cout << "\nData loaded:" << std::endl;
    n_read.print();
    
    // write more data to the file
    n.reset();
    // write data (appends data, works like `save_merged`)
    // the Node tree needs to be compatible with the existing
    // hdf5 state, adding new paths is always fine.  
    n["a/my_data"] = 3.1415;
    n["a/b/c"] = 144;
    // lists are also supported
    n["a/my_list"].append() = 42.0;
    n["a/my_list"].append() = 42;
    
    conduit::relay::io::hdf5_write(n,h5_id);

    // check if a subpath of a list exists
    if(conduit::relay::io::hdf5_has_path(h5_id,"a/my_list/0"))
        std::cout << "\nPath 'myoutput.hdf5:a/my_list/0' exists" << std::endl;

    // Read the entire tree:
    n_read.reset();
    conduit::relay::io::hdf5_read(h5_id,n_read);
    std::cout << "\nData loaded:" << std::endl;
    n_read.print();
    
    // other helpers:
    
    // check if a path is a hdf5 file:
    if(conduit::relay::io::is_hdf5_file("myoutput.hdf5"))
        std::cout << "\nFile 'myoutput.hdf5' is a hdf5 file" << std::endl;
    END_EXAMPLE("relay_io_example_hdf5_interface_1");
}



//-----------------------------------------------------------------------------
TEST(conduit_docs, relay_io_example_hdf5_interface_2)
{
    BEGIN_EXAMPLE("relay_io_example_hdf5_interface_opts");
    Node io_about;
    conduit::relay::io::about(io_about);
    std::cout << "\nRelay I/O Info and Default Options:" << std::endl;
    std::cout << io_about.to_yaml() << std::endl;

    Node &hdf5_opts = io_about["options/hdf5"];
    // change the default chunking threshold to a smaller number
    // to enable compression for a small array
    //
    // Note: These control parameters apply to `bytes` written,
    //       not the number of array elements written
    //
    hdf5_opts["chunking/threshold"]  = 2000;
    hdf5_opts["chunking/chunk_size"] = 2000;

    std::cout << "\nNew HDF5 I/O Options:" << std::endl;
    hdf5_opts.print();
    // set options
    conduit::relay::io::hdf5_set_options(hdf5_opts);

    int num_vals = 5000;
    Node n;
    n["my_values"].set(DataType::float64(num_vals));

    float64 *v_ptr = n["my_values"].value();
    for(int i=0; i< num_vals; i++)
    {
        v_ptr[i] = float64(i);
    }

    // save using options
    std::cout << "\nsaving data to 'myoutput_chunked.hdf5' " << std::endl;
    
    conduit::relay::io::hdf5_save(n,"myoutput_chunked.hdf5");
    END_EXAMPLE("relay_io_example_hdf5_interface_opts");
}


//-----------------------------------------------------------------------------
TEST(conduit_docs, relay_io_example_hdf5_interface_3)
{
    BEGIN_EXAMPLE("relay_io_example_hdf5_interface_read_ndarray");
    // ------------------------------------------------------------------
    // Create a 2D array and show it off.
    int constexpr rank = 2;
    int constexpr nrows = 3;
    int constexpr ncols = 4;
    int constexpr eltcount = nrows * ncols;
    double data[eltcount];
    for (int i = 0; i < eltcount; ++i)
    {
        data[i] = i;
    }

    std::cout << "Array, in memory:\n";
    for (int j = 0; j < nrows; ++j)
    {
        for (int i = 0; i < ncols; ++i)
        {
            std::cout << std::right << std::setw(4) << data[j * ncols + i];
        }
        std::cout << std::endl;
    }

    // Create an HDF5 file with a 2D array.
    herr_t status = 0;
    // HDF5 dimensions are ordered from slowest- to fastest-varying.
    // This is the same as C and C++ nested arrays and opposite from
    // many people's geometric intuition.
    hsize_t hdims[rank]{ nrows, ncols };

    const char* fname = "t_relay_io_hdf5_read_ndarray.hdf5";
    hid_t file = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Create, initialize a dataspace for the dataset
    hid_t    dataset, dataspace;
    dataspace = H5Screate_simple(rank, hdims, NULL);

    // Create, initialize the dataset.  Element type is double.
    const char* dsname = "twoDarray";
    dataset = H5Dcreate(file, dsname, H5T_NATIVE_DOUBLE, dataspace, 
        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    status = H5Dclose(dataset);

    // close the dataspace and file
    status = H5Sclose(dataspace);
    status = H5Fclose(file);
    
    std::cout << "\nsaved array to '" << fname << ":" << dsname << "'" << std::endl;

    // ------------------------------------------------------------------
    // Now read a subset of that 2D array from the HDF5 file.
    // Two rows, two columns; total of four elements.
    int constexpr rnrows = 2;
    int constexpr rncols = 2;
    int constexpr reltcount = rnrows * rncols;
    // As noted earlier, HDF5 orders all dimensions from slowest- to
    // fastest-varying.  In this two-dimensional example, row (or y-index)
    // always comes before column (or x-index).  If working with a 3D
    // dataset, level (or z-index) would come before row.
    int p_sizes[rank]{ rnrows, rncols };
    // offset to row 0, column 1
    int p_offsets[rank]{ 0, 1 };
    // read every row, every other column
    int p_strides[rank]{ 1, 2 };
    // Store pointers to these parameters in the read_opts Node
    Node read_opts;
    read_opts["sizes"].set_external(p_sizes, rank);
    read_opts["offsets"].set_external(p_offsets, rank);
    read_opts["strides"].set_external(p_strides, rank);

    std::cout << "\nHDF5 Options for reading the array:" << std::endl;
    read_opts.print();

    // Read some of the 2D array in the HDF5 file into an array of doubles
    Node read_data;
    double p_data_out[reltcount];
    read_data.set_external(p_data_out, reltcount);
    std::string in_path;
    in_path.append(fname).append(":").append(dsname);
    conduit::relay::io::hdf5_read(in_path.c_str(), read_opts, read_data);

    // Show what we read
    std::cout << "Subset of array, read from '" << in_path << "'" << std::endl;
    for (int j = 0; j < rnrows; ++j)
    {
        for (int i = 0; i < rncols; ++i)
        {
            std::cout << std::right << std::setw(8) << p_data_out[j * rncols + i];
        }
        std::cout << std::endl;
    }
    END_EXAMPLE("relay_io_example_hdf5_interface_read_ndarray");
}
