// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_mpi_test.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay_mpi.hpp"
#include <iostream>
#include "math.h"
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::relay;
using namespace conduit::relay::mpi;


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, conduit_dtype_to_mpi_dtype)
{
    MPI_Datatype dt_test = MPI_DATATYPE_NULL;

    // all support types should be mapped (eg: not MPI_DATATYPE_NULL)


    // signed integers
    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::int8()),
              MPI_DATATYPE_NULL);

    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::int16()),
              MPI_DATATYPE_NULL);

    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::int32()),
              MPI_DATATYPE_NULL);

    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::int64()),
              MPI_DATATYPE_NULL);

    // unsigned integers
    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::uint8()),
              MPI_DATATYPE_NULL);

    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::uint16()),
              MPI_DATATYPE_NULL);

    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::uint32()),
              MPI_DATATYPE_NULL);

    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::uint64()),
              MPI_DATATYPE_NULL);

    // floating point
    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::float32()),
              MPI_DATATYPE_NULL);
    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::float64()),
              MPI_DATATYPE_NULL);

    // string
    EXPECT_NE(conduit_dtype_to_mpi_dtype(DataType::char8_str()),
              MPI_DATATYPE_NULL);

    // empty, object, and list should return null
    EXPECT_EQ(conduit_dtype_to_mpi_dtype(DataType::empty()),
              MPI_DATATYPE_NULL);

    EXPECT_EQ(conduit_dtype_to_mpi_dtype(DataType::object()),
              MPI_DATATYPE_NULL);

    EXPECT_EQ(conduit_dtype_to_mpi_dtype(DataType::object()),
              MPI_DATATYPE_NULL);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, mpi_dtype_to_conduit_dtype)
{
    EXPECT_TRUE(DataType(mpi_dtype_to_conduit_dtype_id(MPI_INT)).is_int());
    EXPECT_TRUE(DataType(mpi_dtype_to_conduit_dtype_id(MPI_UNSIGNED)).is_unsigned_int());

    EXPECT_TRUE(DataType(mpi_dtype_to_conduit_dtype_id(MPI_FLOAT)).is_float());
    EXPECT_TRUE(DataType(mpi_dtype_to_conduit_dtype_id(MPI_DOUBLE)).is_double());
}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, reduce)
{
    Node n_snd, n_reduce;

    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    int root = 0;

    int val = rank * 10;
    n_snd = val;

    mpi::reduce(n_snd, n_reduce, MPI_MAX, root, MPI_COMM_WORLD);

    if(rank == root)
    {
        EXPECT_EQ(n_reduce.as_int(), 10 * (com_size-1));
    }

    // check non-compact case

    n_snd.set(DataType::c_int(2,0,sizeof(int)*2));
    n_reduce.set(DataType::c_int(2,0,sizeof(int)*2));

    int_array snd_vals = n_snd.value();

    snd_vals[0] = 10;
    snd_vals[1] = 20;

    void *reduce_ptr = n_reduce.data_ptr();
    mpi::reduce(n_snd, n_reduce, MPI_SUM, root, MPI_COMM_WORLD);

    if(rank == root)
    {
        int_array reduce_vals = n_reduce.value();

        EXPECT_EQ(reduce_vals[0], 10 * com_size);
        EXPECT_EQ(reduce_vals[1], 20 * com_size);

        // n_reduce should have the same data pointer before and after the reduce
        EXPECT_EQ(reduce_ptr,n_reduce.data_ptr());
    }

    // set to something in-compat
    n_reduce.set(3.1415);

    n_reduce.print();

    Schema s_reduce_pre;

    s_reduce_pre =  n_reduce.schema();

    mpi::reduce(n_snd, n_reduce, MPI_SUM, root, MPI_COMM_WORLD);

    if(rank == root)
    {
        n_reduce.print();

        int_array reduce_vals = n_reduce.value();

        EXPECT_EQ(reduce_vals[0], 10 * com_size);
        EXPECT_EQ(reduce_vals[1], 20 * com_size);

        EXPECT_FALSE(s_reduce_pre.compatible(n_reduce.schema()));
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, allreduce)
{
    Node n_snd, n_reduce;

    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    int val = rank * 10;
    n_snd = val;

    mpi::all_reduce(n_snd, n_reduce, MPI_MAX, MPI_COMM_WORLD);
    EXPECT_EQ(n_reduce.as_int(), 10 * (com_size-1));

    // check non-compact case

    n_snd.set(DataType::c_int(2,0,sizeof(int)*2));
    n_reduce.set(DataType::c_int(2,0,sizeof(int)*2));

    int_array snd_vals = n_snd.value();

    snd_vals[0] = 10;
    snd_vals[1] = 20;

    void *reduce_ptr = n_reduce.data_ptr();
    mpi::all_reduce(n_snd, n_reduce, MPI_SUM, MPI_COMM_WORLD);


    int_array reduce_vals = n_reduce.value();

    EXPECT_EQ(reduce_vals[0], 10 * com_size);
    EXPECT_EQ(reduce_vals[1], 20 * com_size);

    // n_reduce should have the same data pointer before and after the reduce
    EXPECT_EQ(reduce_ptr,n_reduce.data_ptr());


    // set to something in-compat
    n_reduce.set(3.1415);

    n_reduce.print();
    //n_reduce.reset();

    Schema s_reduce_pre;

    s_reduce_pre =  n_reduce.schema();

    mpi::all_reduce(n_snd, n_reduce, MPI_SUM, MPI_COMM_WORLD);


    n_reduce.print();

    reduce_vals = n_reduce.value();

    EXPECT_EQ(reduce_vals[0], 10 * com_size);
    EXPECT_EQ(reduce_vals[1], 20 * com_size);


    EXPECT_FALSE(s_reduce_pre.compatible(n_reduce.schema()));

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, reduce_helpers)
{
    int rank     = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    Node snd(DataType::int64(5));
    Node rcv(DataType::int64(5));

    int64 *snd_vals = snd.value();
    int64 *rcv_vals = rcv.value();

    // sum

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = 10;
    }



    mpi::sum_reduce(snd, rcv, 0, MPI_COMM_WORLD);


    if(rank == 0)
    {
        for(int i=0; i < 5; i++)
        {
            EXPECT_EQ(rcv_vals[i], 10 * com_size);
        }
    }

    // prod

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = 2;
    }


    mpi::prod_reduce(snd, rcv, 0, MPI_COMM_WORLD);


    if(rank == 0)
    {
        for(int i=0; i < 5; i++)
        {
            EXPECT_EQ(rcv_vals[i], pow(com_size,2) );
        }
    }


    // max

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = rank * 10 + 1;
    }

    mpi::max_reduce(snd, rcv, 0, MPI_COMM_WORLD);


    if(rank == 0)
    {
        for(int i=0; i < 5; i++)
        {
            EXPECT_EQ(rcv_vals[i], 10 * (com_size-1) + 1);
        }
    }

    // min

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = rank * 10 + 1;
    }

    mpi::min_reduce(snd, rcv, 0, MPI_COMM_WORLD);

    if(rank == 0)
    {
        for(int i=0; i < 5; i++)
        {
            EXPECT_EQ(rcv_vals[i], 1);
        }
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, all_reduce_helpers)
{
    int rank     = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    Node snd(DataType::int64(5));
    Node rcv(DataType::int64(5));

    int64 *snd_vals = snd.value();
    int64 *rcv_vals = rcv.value();

    // sum

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = 10;
    }

    mpi::sum_all_reduce(snd, rcv, MPI_COMM_WORLD);


    for(int i=0; i < 5; i++)
    {
        EXPECT_EQ(rcv_vals[i], 10 * com_size);
    }

    // prod

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = 2;
    }

    mpi::prod_all_reduce(snd, rcv, MPI_COMM_WORLD);


    for(int i=0; i < 5; i++)
    {
        EXPECT_EQ(rcv_vals[i], pow(com_size,2) );
    }


    // max

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = rank * 10 + 1;
    }


    mpi::max_all_reduce(snd, rcv, MPI_COMM_WORLD);


    for(int i=0; i < 5; i++)
    {
        EXPECT_EQ(rcv_vals[i], 10 * (com_size-1) + 1);
    }

    // min

    for(int i=0; i < 5; i++)
    {
        snd_vals[i] = rank * 10 + 1;
    }


    mpi::min_all_reduce(snd, rcv, MPI_COMM_WORLD);


    for(int i=0; i < 5; i++)
    {
        EXPECT_EQ(rcv_vals[i], 1);
    }

}



//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, send_recv_using_schema)
{
    Node n;
    int rank = mpi::rank(MPI_COMM_WORLD);

    if( rank == 0 )
    {
        n.set(DataType::c_double(3));
        double_array vals = n.value();
        vals[0] = rank + 1;
        vals[1] = 3.4124 * rank;
        vals[2] = 10.7 - rank;

        mpi::send_using_schema(n,1,0,MPI_COMM_WORLD);
    }
    else if( rank == 1 )
    {
        mpi::recv_using_schema(n,0,0,MPI_COMM_WORLD);
    }

    double_array vals = n.value();

    EXPECT_EQ(vals[0], 1);
    EXPECT_EQ(vals[1], 0);
    EXPECT_EQ(vals[2], 10.7);


    n.reset();

    if(rank == 0)
    {
        n["value/a"] = 1;
        n["value/b"] = 2;

        mpi::send_using_schema(n,1,0,MPI_COMM_WORLD);
    }
    else
    {
        mpi::recv_using_schema(n,0,0,MPI_COMM_WORLD);
    }
    int val_a = n["value/a"].to_int();
    int val_b = n["value/b"].to_int();

    EXPECT_EQ(val_a, 1);
    EXPECT_EQ(val_b, 2);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, send_recv_using_schema_any_source_any_tag)
{
    Node n;
    int rank = mpi::rank(MPI_COMM_WORLD);

    if( rank == 0 )
    {
        n.set(DataType::c_double(3));
        double_array vals = n.value();
        vals[0] = rank + 1;
        vals[1] = 3.4124 * rank;
        vals[2] = 10.7 - rank;

        mpi::send_using_schema(n,1,0,MPI_COMM_WORLD);
    }
    else if( rank == 1 )
    {
        // any source any tag
        mpi::recv_using_schema(n,MPI_COMM_WORLD);
    }

    double_array vals = n.value();

    EXPECT_EQ(vals[0], 1);
    EXPECT_EQ(vals[1], 0);
    EXPECT_EQ(vals[2], 10.7);

}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, send_recv_without_using_schema)
{
    Node n;
    int rank = mpi::rank(MPI_COMM_WORLD);

    n.set(DataType::c_double(3));

    if( rank == 0 )
    {
        double_array vals = n.value();
        vals[0] = rank + 1;
        vals[1] = 3.4124 * rank;
        vals[2] = 10.7 - rank;

        mpi::send(n,1,0,MPI_COMM_WORLD);
    }
    else if( rank == 1 )
    {
        mpi::recv(n,0,0,MPI_COMM_WORLD);
    }

    double_array vals = n.value();

    EXPECT_EQ(vals[0], 1);
    EXPECT_EQ(vals[1], 0);
    EXPECT_EQ(vals[2], 10.7);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, send_recv_without_using_schema_any_source_any_tag)
{
    Node n;
    int rank = mpi::rank(MPI_COMM_WORLD);

    n.set(DataType::c_double(3));

    if( rank == 0 )
    {
        double_array vals = n.value();
        vals[0] = rank + 1;
        vals[1] = 3.4124 * rank;
        vals[2] = 10.7 - rank;

        mpi::send(n,1,0,MPI_COMM_WORLD);
    }
    else if( rank == 1 )
    {
        // any source any tag
        mpi::recv(n,MPI_COMM_WORLD);
    }

    double_array vals = n.value();

    EXPECT_EQ(vals[0], 1);
    EXPECT_EQ(vals[1], 0);
    EXPECT_EQ(vals[2], 10.7);
}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, isend_irecv_wait_old_api)
{
    Node n1;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);


    mpi::Request request;

    MPI_Status status;
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &request);
        mpi::wait_recv(&request, &status);
    } else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &request);
        mpi::wait_send(&request, &status);
    }

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, isend_irecv_wait)
{
    Node n1;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);


    mpi::Request request;

    MPI_Status status;
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &request);
    }
    else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &request);
    }

    mpi::wait(&request, &status);

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, isend_irecv_wait_any_source_any_tag)
{
    Node n1;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);


    mpi::Request request;

    MPI_Status status;
    if (rank == 0)
    {
        // any source any tag
        mpi::irecv(n1, MPI_COMM_WORLD, &request);
    }
    else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &request);
    }

    mpi::wait(&request, &status);

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, waitall_old_api)
{
    Node n1;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);



    mpi::Request requests[1];

    MPI_Status statuses[1];
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &requests[0]);
        mpi::wait_all_recv(1, requests, statuses);
    } else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &requests[0]);
        mpi::wait_all_send(1, requests, statuses);
    }

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, waitall)
{
    Node n1;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);

    mpi::Request requests[1];

    MPI_Status statuses[1];
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &requests[0]);
    } else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &requests[0]);
    }

    mpi::wait_all(1, requests, statuses);

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, waitallmultirequest_old_api)
{
    Node n1;
    Node n2;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);

    n2 = 13123;

    mpi::Request requests[2];

    MPI_Status statuses[2];
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &requests[0]);
        mpi::irecv(n2, 1, 0, MPI_COMM_WORLD, &requests[1]);
        mpi::wait_all_recv(2, requests, statuses);
    }
    else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &requests[0]);
        mpi::isend(n2, 0, 0, MPI_COMM_WORLD, &requests[1]);
        mpi::wait_all_send(2, requests, statuses);
    }

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

    EXPECT_EQ(n2.as_int32(), 13123);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, waitallmultirequest)
{
    Node n1;
    Node n2;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles;


    doubles.push_back(rank+1);
    doubles.push_back(3.4124*rank);
    doubles.push_back(10.7 - rank);

    n1.set_external(doubles);

    n2 = 13123;

    mpi::Request requests[2];

    MPI_Status statuses[2];
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &requests[0]);
        mpi::irecv(n2, 1, 0, MPI_COMM_WORLD, &requests[1]);
    }
    else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &requests[0]);
        mpi::isend(n2, 0, 0, MPI_COMM_WORLD, &requests[1]);
    }

    mpi::wait_all(2, requests, statuses);

    EXPECT_EQ(n1.as_float64_ptr()[0], 2);
    EXPECT_EQ(n1.as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1.as_float64_ptr()[2], 9.7);

    EXPECT_EQ(n2.as_int32(), 13123);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, external)
{
    Node n1;
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::vector<double> doubles1;
    std::vector<double> doubles2;
    std::vector<double> doubles3;


    doubles1.push_back(rank+1);
    doubles1.push_back(3.4124*rank);
    doubles1.push_back(10.7 - rank);

    n1.append().set_external(doubles1);

    doubles2.push_back(rank+2);
    doubles2.push_back(3.4124*rank + 1);
    doubles2.push_back(10.7 - rank + 1);

    n1.append().set_external(doubles2);

    doubles3.push_back(rank+3);
    doubles3.push_back(3.4124*rank + 2);
    doubles3.push_back(10.7 - rank + 2);

    n1.append().set_external(doubles3);

    mpi::Request request;

    MPI_Status status;
    if (rank == 0)
    {
        mpi::irecv(n1, 1, 0, MPI_COMM_WORLD, &request);
    }
    else if (rank == 1)
    {
        mpi::isend(n1, 0, 0, MPI_COMM_WORLD, &request);
    }

    mpi::wait(&request, &status);

    EXPECT_EQ(n1[0].as_float64_ptr()[0], 2);
    EXPECT_EQ(n1[0].as_float64_ptr()[1], 3.4124);
    EXPECT_EQ(n1[0].as_float64_ptr()[2], 9.7);

    EXPECT_EQ(n1[1].as_float64_ptr()[0], 3);
    EXPECT_EQ(n1[1].as_float64_ptr()[1], 4.4124);
    EXPECT_EQ(n1[1].as_float64_ptr()[2], 10.7);

    EXPECT_EQ(n1[2].as_float64_ptr()[0], 4);
    EXPECT_EQ(n1[2].as_float64_ptr()[1], 5.4124);
    EXPECT_EQ(n1[2].as_float64_ptr()[2], 11.7);
}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, allgather_simple)
{
    Node n;

    int rank = mpi::rank(MPI_COMM_WORLD);

    n["values/a"] = rank+1;
    n["values/b"] = rank+2;
    n["values/c"] = rank+3;

    Node rcv;
    mpi::all_gather(n,rcv,MPI_COMM_WORLD);
    rcv.print();

    Node res;
    res.set_external((int*)rcv.data_ptr(), 6);
    res.print();

    int *res_ptr = res.value();
    EXPECT_EQ(res_ptr[0],1);
    EXPECT_EQ(res_ptr[1],2);
    EXPECT_EQ(res_ptr[2],3);
    EXPECT_EQ(res_ptr[3],2);
    EXPECT_EQ(res_ptr[4],3);
    EXPECT_EQ(res_ptr[5],4);

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, gather_simple)
{
    Node n;

    int rank = mpi::rank(MPI_COMM_WORLD);

    n["values/a"] = rank+1;
    n["values/b"] = rank+2;
    n["values/c"] = rank+3;

    Node rcv;
    mpi::all_gather(n,rcv,MPI_COMM_WORLD);
    rcv.print();

    if(rank == 0)
    {
        Node res;
        res.set_external((int*)rcv.data_ptr(), 6);
        res.print();

        int *res_ptr = res.value();
        EXPECT_EQ(res_ptr[0],1);
        EXPECT_EQ(res_ptr[1],2);
        EXPECT_EQ(res_ptr[2],3);
        EXPECT_EQ(res_ptr[3],2);
        EXPECT_EQ(res_ptr[4],3);
        EXPECT_EQ(res_ptr[5],4);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, gather_using_schema_simple)
{
    Node n;

    int rank = mpi::rank(MPI_COMM_WORLD);

    n["values/a"] = rank+1;
    n["values/b"] = rank+2;
    n["values/c"] = rank+3;
    if(rank != 0)
    {
        n["values/d"] = rank+4;
    }

    Node rcv;
    mpi::gather_using_schema(n,rcv,0,MPI_COMM_WORLD);
    rcv.print();

    if( rank == 0)
    {
        Node res;
        res.set_external((int*)rcv.data_ptr(), 7);
        res.print();

        int *res_ptr = res.value();
        EXPECT_EQ(res_ptr[0],1);
        EXPECT_EQ(res_ptr[1],2);
        EXPECT_EQ(res_ptr[2],3);
        EXPECT_EQ(res_ptr[3],2);
        EXPECT_EQ(res_ptr[4],3);
        EXPECT_EQ(res_ptr[5],4);
        EXPECT_EQ(res_ptr[6],5);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, all_gather_using_schema_simple)
{
    Node n;

    int rank = mpi::rank(MPI_COMM_WORLD);

    n["values/a"] = rank+1;
    n["values/b"] = rank+2;
    n["values/c"] = rank+3;
    if(rank != 0)
    {
        n["values/d"] = rank+4;
    }

    Node rcv;
    mpi::all_gather_using_schema(n,rcv,MPI_COMM_WORLD);
    rcv.print();

    Node res;
    res.set_external((int*)rcv.data_ptr(), 7);
    res.print();

    int *res_ptr = res.value();
    EXPECT_EQ(res_ptr[0],1);
    EXPECT_EQ(res_ptr[1],2);
    EXPECT_EQ(res_ptr[2],3);
    EXPECT_EQ(res_ptr[3],2);
    EXPECT_EQ(res_ptr[4],3);
    EXPECT_EQ(res_ptr[5],4);
    EXPECT_EQ(res_ptr[6],5);

}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, bcast)
{
    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    for(int root = 0; root < com_size; root++)
    {
        Node n;
        n.set(DataType::int64(3));

        std::vector<int64> vals;
        if(rank == root)
        {
            int64_array vals = n.value();
            vals[0] = 11;
            vals[1] = 22;
            vals[2] = 33;
        }

        mpi::broadcast(n,root,MPI_COMM_WORLD);

        int64 *vals_ptr = n.value();

        EXPECT_EQ(vals_ptr[0], 11);
        EXPECT_EQ(vals_ptr[1], 22);
        EXPECT_EQ(vals_ptr[2], 33);

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }


    for(int root = 0; root < com_size; root++)
    {
        Node n;

        if(rank == root)
        {
            n["a/b/c/d/e/f"].set_int64(10);
        }
        else
        {
            n["a/b/c/d/e/f"].set_int64(0);
        }

        mpi::broadcast(n,root,MPI_COMM_WORLD);

        int64 val = n["a/b/c/d/e/f"].value();

        EXPECT_EQ(val, 10);

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }


    for(int root = 0; root < com_size; root++)
    {
        Node n;

        if(rank == root)
        {
            n["a/b/c/d/e/f"] = "g";
        }
        else
        {
            n["a/b/c/d/e/f"] = "f";
        }

        mpi::broadcast(n,root,MPI_COMM_WORLD);

        std::string val = n["a/b/c/d/e/f"].as_string();

        EXPECT_EQ(val, "g");

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, bcast_using_schema)
{

    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    for(int root = 0; root < com_size; root++)
    {
        Node n;

        std::vector<int64> vals;
        if(rank == root)
        {
            vals.push_back(11);
            vals.push_back(22);
            vals.push_back(33);

            n.set_external(vals);
        }

        mpi::broadcast_using_schema(n,root,MPI_COMM_WORLD);

        int64 *vals_ptr = n.value();

        EXPECT_EQ(vals_ptr[0], 11);
        EXPECT_EQ(vals_ptr[1], 22);
        EXPECT_EQ(vals_ptr[2], 33);

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }


    for(int root = 0; root < com_size; root++)
    {
        Node n;

        if(rank == root)
        {
            n["a/b/c/d/e/f"].set_int64(10);
        }

        mpi::broadcast_using_schema(n,root,MPI_COMM_WORLD);

        int64 val = n["a/b/c/d/e/f"].value();

        EXPECT_EQ(val, 10);

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }


    for(int root = 0; root < com_size; root++)
    {
        Node n;

        if(rank == root)
        {
            n["a/b/c/d/e/f"] = "g";
        }

        mpi::broadcast_using_schema(n,root,MPI_COMM_WORLD);

        std::string val = n["a/b/c/d/e/f"].as_string();

        EXPECT_EQ(val, "g");

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }


    for(int root = 0; root < com_size; root++)
    {
        Node n;

        if(rank == root)
        {
            n["value/a"] = 1;
            n["value/b"] = 2;
        }
        else
        {
            n["value/a"] = -1;
            n["value/b"] = -1;
        }

        mpi::broadcast_using_schema(n,root,MPI_COMM_WORLD);

        int val_a = n["value/a"].to_int();
        int val_b = n["value/b"].to_int();


        EXPECT_EQ(val_a, 1);
        EXPECT_EQ(val_b, 2);

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }


}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, bcast_using_schema_non_empty_node)
{

    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    for(int root = 0; root < com_size; root++)
    {
        Node n;

        std::vector<int64> vals;
        if(rank == root)
        {
            vals.push_back(11);
            vals.push_back(22);
            vals.push_back(33);

            n["here"].set(vals);
        }
        else
        {
            n["there"] = "here is a string";
        }

        mpi::broadcast_using_schema(n,root,MPI_COMM_WORLD);
        n.print();
        int64 *vals_ptr = n["here"].value();

        EXPECT_EQ(vals_ptr[0], 11);
        EXPECT_EQ(vals_ptr[1], 22);
        EXPECT_EQ(vals_ptr[2], 33);

        CONDUIT_INFO("Bcast from root = "
                     << root  << "\n"
                     << "rank: " << rank << " res = "
                     << n.to_json());
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, reuse_of_node_as_input)
{
    // this test tests out specifics with
    // compatible schemas

    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    int value = 0;
    std::string msg;

    if(rank == 0)
    {
        value = 1;
        msg = "HI!";
    }

    Node n_local, n_global;
    n_local.set(value);
    relay::mpi::sum_all_reduce(n_local,
                               n_global,
                               MPI_COMM_WORLD);
    value  = n_global.as_int();

    if(value == 1)
    {

        n_global.set(msg);
        conduit::relay::mpi::broadcast_using_schema(n_global,
                                                    0,
                                                    MPI_COMM_WORLD);
        msg = n_global.as_string();
    }

    EXPECT_EQ(value,1);
    EXPECT_EQ(msg,"HI!");
}


//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, reduce_compat_check)
{
    // this test tests out specifics with
    // compatible schemas

    int rank = mpi::rank(MPI_COMM_WORLD);
    int com_size = mpi::size(MPI_COMM_WORLD);

    int value = 0;

    if(rank == 0)
    {
        value = 1;
    }

    Node n_local, n_global;
    n_local.set(value);
    relay::mpi::sum_all_reduce(n_local,
                               n_global,
                               MPI_COMM_WORLD);
    value  = n_global.as_int();

    // now all gather
    relay::mpi::all_gather(n_local,
                           n_global,
                           MPI_COMM_WORLD);

    n_global.print();
    Node n_expected, info;
    n_expected.append() = 1;
    n_expected.append() = 0;

    EXPECT_FALSE(n_global.diff(n_expected,info));

}

//-----------------------------------------------------------------------------
TEST(conduit_mpi_test, mem_info)
{
    Node alloc_help, usage_prev, stats_prev, usage_curr, stats_curr;

    int rank = mpi::rank(MPI_COMM_WORLD);

    relay::mpi::memory_usage(usage_prev,MPI_COMM_WORLD);
    relay::mpi::memory_stats(stats_prev,MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == 0)
    {
        // 5 mb
        alloc_help.set(DataType::uint8(5*1024*1024));
        alloc_help.print();
    }

    if (rank == 1)
    {
        // 15 mB -- 10 extra than rank 0
        alloc_help.set(DataType::uint8(15*1024*1024));

        alloc_help.print();
    }

    // we have to touch the memory to make sure
    // it is reported as used
    uint8_array vals =alloc_help.value();
    for(index_t i=0;i<vals.number_of_elements();i++)
    {
        vals[i] = i % 256;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    relay::mpi::memory_usage(usage_curr,MPI_COMM_WORLD);
    relay::mpi::memory_stats(stats_curr,MPI_COMM_WORLD);

    if(rank == 0)
    {
        std::cout << "rank 0:\n"
                  << "mem_usage - prev:\n" << usage_prev.to_yaml() << "\n"
                  << "mem_usage - curr:\n" << usage_curr.to_yaml() << "\n"
                  << "mem_stats - prev:\n" << stats_prev.to_yaml() << "\n"
                  << "mem_stats - curr:\n" << stats_curr.to_yaml() << "\n"
                  << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == 1)
    {

        std::cout << "rank 1:\n"
                  << "mem_usage - prev:\n" << usage_prev.to_yaml() << "\n"
                  << "mem_usage - curr:\n" << usage_curr.to_yaml() << "\n"
                  << "mem_stats - prev:\n" << stats_prev.to_yaml() << "\n"
                  << "mem_stats - curr:\n" << stats_curr.to_yaml() << "\n"
                  << std::endl;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    uint64_array uprev_vals = usage_prev.value();
    uint64_array ucurr_vals = usage_curr.value();

    // curr should be higher than prev
    EXPECT_GT(ucurr_vals[0],uprev_vals[0]);
    EXPECT_GT(ucurr_vals[1],uprev_vals[1]);
    // rank 1 should be higher than rank 0
    EXPECT_GT(ucurr_vals[1],ucurr_vals[0]);

    // curr should be higher than prev
    EXPECT_GT(stats_curr["min"].to_uint64(), stats_prev["min"].to_uint64());
    EXPECT_GT(stats_curr["max"].to_uint64(), stats_prev["max"].to_uint64());

    // expect max to be at least 8 mB above min
    EXPECT_GT(stats_curr["max"].to_uint64() -stats_prev["max"].to_uint64(),
              1024*8); // results are all kB

    // mean should be higher than rank 0
    EXPECT_GT(stats_curr["mean"].to_uint64(), ucurr_vals[0]);

}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}

