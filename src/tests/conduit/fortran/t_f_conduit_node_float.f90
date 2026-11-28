!* Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
!* Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
!* other details. No copyright assignment is required to contribute to Conduit.

!------------------------------------------------------------------------------
!
! f_conduit_node_float64.f
!
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
module f_conduit_node_float64
!------------------------------------------------------------------------------

  use iso_c_binding
  use fruit
  use conduit
  use conduit_endianness_enum
  implicit none

!------------------------------------------------------------------------------
contains
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------    
! float32 tests
!------------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine t_node_set_float32
        type(C_PTR) cnode
        real(4) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_float32")
        !----------------------------------------------------------------------

        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        cnode = conduit_node_create()
        ! n.set_float32(3.1415);
        call conduit_node_set_float32(cnode,3.1415)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 res = n.as_float32();
        res = conduit_node_as_float32(cnode)
        call assert_equals (3.1415, res)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_float32

    !--------------------------------------------------------------------------
    subroutine t_node_set_float32_ptr
        type(C_PTR) cnode
        real(4), dimension(5) :: data
        integer nele
        integer i
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_float32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! Node n.set_float32_ptr(data,5);
        call conduit_node_set_float32_ptr(cnode,data,5_8)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! index_t nele = n.dtype().number_of_elements();
        nele = conduit_node_number_of_elements(cnode)
        call assert_equals(nele,5)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_float32_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_set_external_float32_ptr
        type(C_PTR) cnode
        real(4), dimension(5), target :: data
        real(4) res
        integer i
        real(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_external_float32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float32_ptr(data,5);
        call conduit_node_set_external_float32_ptr(cnode,data,5_8)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 res = n.as_float32()
        res = conduit_node_as_float32(cnode)
        call assert_equals(res,3.1415)
        ! float32 *res_ptr = n.as_float32_ptr()
        call conduit_node_as_float32_ptr(cnode,f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_external_float32_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_set_external_float32_ptr_detailed
        type(C_PTR) cnode
        real(4), dimension(5), target :: data
        real(4) res
        integer i
        real(4), pointer :: f_arr(:)
        integer(C_SIZE_T) endianness
        endianness = int(conduit_endianness_default_id,8)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_external_float32_ptr_detailed")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float32_ptr(data,5,0,4,4,Endianness::DEFAULT_ID);
        call conduit_node_set_external_float32_ptr_detailed(cnode,data,5_8, 0_8, 4_8, 4_8,endianness)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 res = n.as_float32()
        res = conduit_node_as_float32(cnode)
        call assert_equals(res,3.1415)
        ! float32 *res_ptr = n.as_float32_ptr()
        call conduit_node_as_float32_ptr(cnode,f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
            ! set_external(...) semantics passing a pointer -- mem addys should be the same 
            if (loc(f_arr(i)) /= loc(data(i))) then
              call add_fail("Same addresses expected")
            endif
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_external_float32_ptr_detailed

    !--------------------------------------------------------------------------
    subroutine t_node_as_float32
        type(C_PTR) cnode
        type(C_PTR) n1
        real(4) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_as_float32")
        !----------------------------------------------------------------------
                
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_float32(3.1415)
        call conduit_node_set_float32(cnode,3.1415)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 res = n.as_float32();
        res = conduit_node_as_float32(cnode)
        call assert_equals (3.1415, res)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_as_float32

    !--------------------------------------------------------------------------
    subroutine t_node_as_float32_ptr
        type(C_PTR) cnode
        real(4), dimension(5) :: data
        integer nele
        integer i
        real(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_as_float32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float32_ptr(data,5);
        call conduit_node_set_float32_ptr(cnode,data,5_8)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! index_t nele = n.dtype().number_of_elements();
        nele = conduit_node_number_of_elements(cnode)
        call assert_equals(nele,5)
        ! float32 *res_ptr = n.as_float32_ptr()
        call conduit_node_as_float32_ptr(cnode,f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_as_float32_ptr


    !--------------------------------------------------------------------------
    subroutine t_node_as_float32_ptr_read_scalar
        type(C_PTR) cnode
        integer nele
        integer i
        real(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_as_float32_ptr_read_scalar")
        !----------------------------------------------------------------------
                
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_float32(3.1415);
        call conduit_node_set_float32(cnode,3.1415)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! index_t nele = n.dtype().number_of_elements();
        nele = conduit_node_number_of_elements(cnode)
        call assert_equals(nele,1)
        ! float32 *ptr = n.as_float32_ptr();
        call conduit_node_as_float32_ptr(cnode,f_arr)
        ! check if ptr[0] == 3.1415
        call assert_equals(f_arr(1),3.1415)

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_as_float32_ptr_read_scalar


    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_float32
        type(C_PTR) cnode
        type(C_PTR) n1
        real(4) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_float32")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;     
        cnode = conduit_node_create()
        ! n["my_sub"].set_float32(3.1415)
        ! // or
        ! n.set_path_float32("my_sub",3.1415)
        call conduit_node_set_path_float32(cnode,"my_sub",3.1415)
        ! n.print_detailed()
        call conduit_node_print_detailed(cnode)
        ! float32 res = n["my_sub"].as_float32();
        res = conduit_node_fetch_path_as_float32(cnode,"my_sub")
        call assert_equals (3.1415, res)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_float32


    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_float32_ptr
        type(C_PTR) cnode
        real(4), dimension(5) :: data
        real(4) res
        integer i
        real(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_float32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float32_ptr(data,5);
        call conduit_node_set_path_float32_ptr(cnode,"my_sub",data,5_8)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 *res_ptr = n.as_float32_ptr()
        call conduit_node_fetch_path_as_float32_ptr(cnode,"my_sub",f_arr)
        
        call assert_equals(size(data),size(f_arr));

        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_float32_ptr


    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_external_float32_ptr
        type(C_PTR) cnode
        real(4), dimension(5), target :: data
        real(4) res
        integer i
        real(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_external_float32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float32_ptr(data,5);
        call conduit_node_set_path_external_float32_ptr(cnode,"my_sub",data,5_8)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 res = n.as_float32()
        res = conduit_node_fetch_path_as_float32(cnode,"my_sub")
        call assert_equals(res,3.1415)
        ! float32 *res_ptr = n.as_float32_ptr()
        call conduit_node_fetch_path_as_float32_ptr(cnode,"my_sub",f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_external_float32_ptr
    
    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_external_float32_ptr_detailed
        type(C_PTR) cnode
        real(4), dimension(5), target :: data
        real(4) res
        integer i
        real(4), pointer :: f_arr(:)

        integer(C_SIZE_T) endianness
        endianness = int(conduit_endianness_default_id,8)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_external_float32_ptr_detailed")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_path_external_float32_ptr("my_sub",data,5,0,4,4,Endianness::DEFAULT_ID5);
        call conduit_node_set_path_external_float32_ptr_detailed(cnode,"my_sub",data,5_8, 0_8, 4_8, 4_8,endianness)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float32 res = n.as_float32()
        res = conduit_node_fetch_path_as_float32(cnode,"my_sub")
        call assert_equals(res,3.1415)
        ! float32 *res_ptr = n.as_float32_ptr()
        call conduit_node_fetch_path_as_float32_ptr(cnode,"my_sub",f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
            ! set_external(...) semantics passing a pointer -- mem addys should be the same 
            if (loc(f_arr(i)) /= loc(data(i))) then
              call add_fail("Same addresses expected")
            endif
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_external_float32_ptr_detailed
    
!------------------------------------------------------------------------------    
! float64 tests
!------------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine t_node_set_float64
        type(C_PTR) cnode
        real(8) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_float64")
        !----------------------------------------------------------------------

        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        cnode = conduit_node_create()
        ! n.set_float64(3.1415d+0);
        call conduit_node_set_float64(cnode,3.1415d+0)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 res = n.as_float64();
        res = conduit_node_as_float64(cnode)
        call assert_equals (3.1415d+0, res)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_float64

    !--------------------------------------------------------------------------
    subroutine t_node_set_float64_ptr
        type(C_PTR) cnode
        real(8), dimension(5) :: data
        integer nele
        integer i
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_float64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! Node n.set_float64_ptr(data,5);
        call conduit_node_set_float64_ptr(cnode,data,5_8)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! index_t nele = n.dtype().number_of_elements();
        nele = conduit_node_number_of_elements(cnode)
        call assert_equals(nele,5)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_float64_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_set_external_float64_ptr
        type(C_PTR) cnode
        real(8), dimension(5), target :: data
        real(8) res
        integer i
        real(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_external_float64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float64_ptr(data,5);
        call conduit_node_set_external_float64_ptr(cnode,data,5_8)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415d+0
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 res = n.as_float64()
        res = conduit_node_as_float64(cnode)
        call assert_equals(res,3.1415d+0)
        ! float64 *res_ptr = n.as_float64_ptr()
        call conduit_node_as_float64_ptr(cnode,f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415d+0)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_external_float64_ptr
    
    !--------------------------------------------------------------------------
    subroutine t_node_set_external_float64_ptr_detailed
        type(C_PTR) cnode
        real(8), dimension(5), target :: data
        real(8) res
        integer i
        real(8), pointer :: f_arr(:)
        integer(C_SIZE_T) endianness
        endianness = int(conduit_endianness_default_id,8)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_external_float64_ptr_detailed")
        !----------------------------------------------------------------------
        
        ! fill our 64-bit x5 float array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float64_ptr(data,5,0,8,8,Endianness::DEFAULT_ID5);
        call conduit_node_set_external_float64_ptr_detailed(cnode,data, 5_8, 0_8, 8_8, 8_8, endianness)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415d+0
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 res = n.as_float64()
        res = conduit_node_as_float64(cnode)
        call assert_equals(res,3.1415d+0)
        ! float64 *res_ptr = n.as_float64_ptr()
        call conduit_node_as_float64_ptr(cnode,f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415d+0)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
            ! set_external(...) semantics passing a pointer -- mem addys should be the same 
            if (loc(f_arr(i)) /= loc(data(i))) then
              call add_fail("Same addresses expected")
            endif
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_external_float64_ptr_detailed

    !--------------------------------------------------------------------------
    subroutine t_node_as_float64
        type(C_PTR) cnode
        type(C_PTR) n1
        real(8) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_as_float64")
        !----------------------------------------------------------------------
                
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_float64(3.1415d+0)
        call conduit_node_set_float64(cnode,3.1415d+0)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 res = n.as_float64();
        res = conduit_node_as_float64(cnode)
        call assert_equals (3.1415d+0, res)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_as_float64

    !--------------------------------------------------------------------------
    subroutine t_node_as_float64_ptr
        type(C_PTR) cnode
        real(8), dimension(5) :: data
        integer nele
        integer i
        real(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_as_float64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float64_ptr(data,5);
        call conduit_node_set_float64_ptr(cnode,data,5_8)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! index_t nele = n.dtype().number_of_elements();
        nele = conduit_node_number_of_elements(cnode)
        call assert_equals(nele,5)
        ! float64 *res_ptr = n.as_float64_ptr()
        call conduit_node_as_float64_ptr(cnode,f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_as_float64_ptr


    !--------------------------------------------------------------------------
    subroutine t_node_as_float64_ptr_read_scalar
        type(C_PTR) cnode
        integer nele
        integer i
        real(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_as_float64_ptr_read_scalar")
        !----------------------------------------------------------------------
                
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_float64(3.1415d+0);
        call conduit_node_set_float64(cnode,3.1415d+0)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! index_t nele = n.dtype().number_of_elements();
        nele = conduit_node_number_of_elements(cnode)
        call assert_equals(nele,1)
        ! float64 *ptr = n.as_float64_ptr();
        call conduit_node_as_float64_ptr(cnode,f_arr)
        ! check if ptr[0] == 3.1415d+0
        call assert_equals(f_arr(1),3.1415d+0)

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_as_float64_ptr_read_scalar


    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_float64
        type(C_PTR) cnode
        type(C_PTR) n1
        real(8) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_float64")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;     
        cnode = conduit_node_create()
        ! n["my_sub"].set_float64(3.1415d+0)
        ! // or
        ! n.set_path_float64("my_sub",3.1415d+0)
        call conduit_node_set_path_float64(cnode,"my_sub",3.1415d+0)
        ! n.print_detailed()
        call conduit_node_print_detailed(cnode)
        ! float64 res = n["my_sub"].as_float64();
        res = conduit_node_fetch_path_as_float64(cnode,"my_sub")
        call assert_equals (3.1415d+0, res)
        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_float64


    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_float64_ptr
        type(C_PTR) cnode
        real(8), dimension(5) :: data
        real(8) res
        integer i
        real(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_float64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float64_ptr(data,5);
        call conduit_node_set_path_float64_ptr(cnode,"my_sub",data,5_8)
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 *res_ptr = n.as_float64_ptr()
        call conduit_node_fetch_path_as_float64_ptr(cnode,"my_sub",f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));

        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo


        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_float64_ptr


    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_external_float64_ptr
        type(C_PTR) cnode
        real(8), dimension(5), target :: data
        real(8) res
        integer i
        real(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_external_float64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_external_float64_ptr(data,5);
        call conduit_node_set_path_external_float64_ptr(cnode,"my_sub",data,5_8)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415d+0
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 res = n.as_float64()
        res = conduit_node_fetch_path_as_float64(cnode,"my_sub")
        call assert_equals(res,3.1415d+0)
        ! float64 *res_ptr = n.as_float64_ptr()
        call conduit_node_fetch_path_as_float64_ptr(cnode,"my_sub",f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415d+0)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_external_float64_ptr
    
    !--------------------------------------------------------------------------
    subroutine t_node_set_and_fetch_path_external_float64_ptr_detailed
        type(C_PTR) cnode
        real(8), dimension(5), target :: data
        real(8) res
        integer i
        real(8), pointer :: f_arr(:)

        integer(C_SIZE_T) endianness
        endianness = int(conduit_endianness_default_id,8)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_and_fetch_path_external_float64_ptr_detailed")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        cnode = conduit_node_create()
        ! n.set_path_external_float64_ptr("my_sub",data,5,0,8,8,Endianness::DEFAULT_ID5);
        call conduit_node_set_path_external_float64_ptr_detailed(cnode,"my_sub",data,5_8, 0_8, 8_8, 8_8,endianness)
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 3.1415d+0
        ! n.print_detailed();
        call conduit_node_print_detailed(cnode)
        ! float64 res = n.as_float64()
        res = conduit_node_fetch_path_as_float64(cnode,"my_sub")
        call assert_equals(res,3.1415d+0)
        ! float64 *res_ptr = n.as_float64_ptr()
        call conduit_node_fetch_path_as_float64_ptr(cnode,"my_sub",f_arr)
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        call assert_equals(f_arr(1),3.1415d+0)
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
            ! set_external(...) semantics passing a pointer -- mem addys should be the same 
            if (loc(f_arr(i)) /= loc(data(i))) then
              call add_fail("Same addresses expected")
            endif
        enddo

        call conduit_node_destroy(cnode)
        
    end subroutine t_node_set_and_fetch_path_external_float64_ptr_detailed


!------------------------------------------------------------------------------
end module f_conduit_node_float64
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
program fortran_test
!------------------------------------------------------------------------------
  use fruit
  use f_conduit_node_float64
  implicit none
  logical ok
  
  call init_fruit

  !----------------------------------------------------------------------------
  ! call our test routines
  !----------------------------------------------------------------------------
  call t_node_set_float32
  call t_node_set_float32_ptr
  call t_node_set_external_float32_ptr
  call t_node_set_external_float32_ptr_detailed

  call t_node_as_float32
  call t_node_as_float32_ptr
  call t_node_as_float32_ptr_read_scalar
 
  call t_node_set_and_fetch_path_float32
  call t_node_set_and_fetch_path_float32_ptr
  call t_node_set_and_fetch_path_external_float32_ptr
  call t_node_set_and_fetch_path_external_float32_ptr_detailed

  call t_node_set_float64
  call t_node_set_float64_ptr
  call t_node_set_external_float64_ptr
  call t_node_set_external_float64_ptr_detailed

  call t_node_as_float64
  call t_node_as_float64_ptr
  call t_node_as_float64_ptr_read_scalar
 
  call t_node_set_and_fetch_path_float64
  call t_node_set_and_fetch_path_float64_ptr
  call t_node_set_and_fetch_path_external_float64_ptr
  call t_node_set_and_fetch_path_external_float64_ptr_detailed
    
  
  call fruit_summary
  call fruit_finalize
  call is_all_successful(ok)
  
  if (.not. ok) then
     call exit(1)
  endif

!------------------------------------------------------------------------------
end program fortran_test
!------------------------------------------------------------------------------


