!* Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
!* Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
!* other details. No copyright assignment is required to contribute to Conduit.

!------------------------------------------------------------------------------
!
! f_conduit_node_obj.f
!
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
module f_conduit_node_obj
!------------------------------------------------------------------------------
  use iso_c_binding
  use fruit
  use conduit
  use conduit_obj
  implicit none

!------------------------------------------------------------------------------
contains
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
!  Obj Style Tests
!------------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    subroutine t_node_obj_create
        type(node) obj
        type(node) info_obj
        integer res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_create")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        ! n.print_detailed();
        obj = conduit_node_obj_create()
        call obj%print_detailed()
        call assert_true(logical(obj%is_root() .eqv. .true. ))
        ! Node n_info;
        ! n.info(n_info);
        ! n_info.print();
        info_obj = conduit_node_obj_create()
        call obj%info(info_obj)
        call info_obj%print() 
        call conduit_node_obj_destroy(obj)
        call conduit_node_obj_destroy(info_obj)
    
    end subroutine t_node_obj_create

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_int
        type(node) obj
        integer res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_int")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;    
        obj = conduit_node_obj_create()
        ! n.set(42);
        call obj%set_int(42)
        ! n.print_detailed();
        call obj%print_detailed()
        ! int res = n.as_int();
        res = obj%as_int()
        call assert_equals (42, res)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_int


    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_int32
        type(node) obj
        integer(4) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_int32")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! n.set_int32(42);
        call obj%set_int32(42)
        ! n.print_detailed();
        call obj%print_detailed()
        ! int32 res = n.as_int32();
        res = obj%as_int32()
        call assert_equals (42, res)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_int32

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_double
        type(node) obj
        real(kind=8) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_double")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! n.set(3.1415);
        call obj%set_double(3.1415d+0)
        ! n.print_detailed();
        call obj%print_detailed()
        ! double res = n.as_double();
        res = obj%as_double()
        call assert_equals(3.1415d+0, res)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_double

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_float64
        type(node) obj
        real(kind=8) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_float64")
        !----------------------------------------------------------------------

        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! n.set_float64(3.1415);
        call obj%set_float64(3.1415d+0)
        ! n.print_detailed();
        call obj%print_detailed()
        ! float64 res = n.as_float64();
        res = obj%as_float64()
        call assert_equals(3.1415d+0, res)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_float64

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_and_fetch_path_int32_ptr
        type(node) n
        integer(4), dimension(5) :: data
        integer i
        integer(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_and_fetch_path_int32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        ! create node and set data in the tree
        n = conduit_node_obj_create()
        call n%set_path_int32_ptr("sub/path/test",data,5_8)

        ! fetch pointer to the data from comparison 
        call n%fetch_path_as_int32_ptr("sub/path/test",f_arr)
        
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo
        ! cleanup
        call conduit_node_obj_destroy(n)
    end subroutine t_node_obj_set_and_fetch_path_int32_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_and_fetch_path_int64_ptr
        type(node) n
        integer(8), dimension(5) :: data
        integer i
        integer(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_and_fetch_path_int64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        ! create node and set data in the tree
        n = conduit_node_obj_create()
        call n%set_path_int64_ptr("sub/path/test",data,5_8)

        ! fetch pointer to the data from comparison 
        call n%fetch_path_as_int64_ptr("sub/path/test",f_arr)
        
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        !=======
        ! NOTE:  fruit doesn't support assert_equals with integer(8)
        !=======
        ! check array value equiv
        do i = 1,5
            call assert_equals(int(f_arr(i),4),int(data(i),4))
        enddo
        ! cleanup
        call conduit_node_obj_destroy(n)
    end subroutine t_node_obj_set_and_fetch_path_int64_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_and_fetch_path_float32_ptr
        type(node) n
        real(4), dimension(5) :: data
        integer i
        real(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_and_fetch_path_float32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        ! create node and set data in the tree
        n = conduit_node_obj_create()
        call n%set_path_float32_ptr("sub/path/test",data,5_8)

        ! fetch pointer to the data from comparison 
        call n%fetch_path_as_float32_ptr("sub/path/test",f_arr)
        
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo
        ! cleanup
        call conduit_node_obj_destroy(n)
    end subroutine t_node_obj_set_and_fetch_path_float32_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_and_fetch_path_float64_ptr
        type(node) n
        real(8), dimension(5) :: data
        integer i
        real(8), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_and_fetch_path_float64_ptr")
        !----------------------------------------------------------------------
        
        ! fill our array
        do i = 1,5
            data(i) = i
        enddo
        ! create node and set data in the tree
        n = conduit_node_obj_create()
        call n%set_path_float64_ptr("sub/path/test",data,5_8)

        ! fetch pointer to the data from comparison 
        call n%fetch_path_as_float64_ptr("sub/path/test",f_arr)
        
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));
        
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo
        ! cleanup
        call conduit_node_obj_destroy(n)
    end subroutine t_node_obj_set_and_fetch_path_float64_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_int32_ptr
        type(node) obj
        integer(4), dimension(5) :: data
        integer nele
        integer i
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_int32_ptr")
        !----------------------------------------------------------------------

        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        obj = conduit_node_obj_create()
        call obj%print_detailed()
        ! Node n.set_int32_ptr(data,5);
        call obj%set_int32_ptr(data,5_8)
        ! n.print_detailed();
        call obj%print_detailed()
        ! index_t nele = n.dtype().number_of_elements();
        nele = obj%number_of_elements()
        call assert_equals(nele,5)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_int32_ptr
    
    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_external_int32_ptr
        type(node) obj
        integer(4), dimension(5), target :: data
        integer res
        integer i
        integer(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_external_int32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;          
        obj = conduit_node_obj_create()
        ! n.set_external_int32_ptr(data,5);
        call obj%set_external_int32_ptr(data,5_8)
        call assert_true( logical(obj%is_data_external() .eqv. .true. ))
        ! change the first element in the array
        ! so we can check the external semantics
        data(1) = 42
        ! n.print_detailed();
        call obj%print_detailed()
        ! int32 res = n.as_int32()
        res = obj%as_int32()
        call assert_equals(res,42)
        ! int32 *res_ptr = n.as_int32_ptr()
        call obj%as_int32_ptr(f_arr)

        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));

        call assert_equals(f_arr(1),42)
        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo

        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_external_int32_ptr
    
    !--------------------------------------------------------------------------
    subroutine t_node_obj_as_int32_ptr
        type(node) obj
        integer(4), dimension(5) :: data
        integer nele
        integer i
        integer(4), pointer :: f_arr(:)
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_as_int32_ptr")
        !----------------------------------------------------------------------
        
        ! fill our 32-bit x5 integer array
        do i = 1,5
            data(i) = i
        enddo
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n; 
        obj = conduit_node_obj_create()
        ! n.set_external_int32_ptr(data,5);
        call obj%set_int32_ptr(data,5_8)
        ! n.print_detailed();
        call obj%print_detailed()
        ! index_t nele = n.dtype().number_of_elements();
        nele = obj%number_of_elements()
        call assert_equals(nele,5)
        ! int32 *res_ptr = n.as_int32_ptr()
        call obj%as_int32_ptr(f_arr)
        
        ! check size of fetched array
        call assert_equals(size(data),size(f_arr));

        ! check array value equiv
        do i = 1,5
            call assert_equals(f_arr(i),data(i))
        enddo
        
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_as_int32_ptr
    
    !--------------------------------------------------------------------------
    subroutine t_node_obj_as_int32_ptr_read_scalar
        type(node) obj
        integer nele
        integer i
        integer(4), pointer :: f_arr(:)

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_as_int32_ptr_read_scalar")
        !----------------------------------------------------------------------

        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! n.set_int32(42);
        call obj%set_int32(42)
        ! n.print_detailed();
        call obj%print_detailed()
        
        ! index_t nele = n.dtype().number_of_elements();
        nele = obj%number_of_elements()
        call assert_equals(nele,1)
        ! int32 *ptr = n.as_int32_ptr();
        call obj%as_int32_ptr(f_arr)
        ! check if ptr[0] == 42
        call assert_equals(f_arr(1),42)

        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_as_int32_ptr_read_scalar
    
    !--------------------------------------------------------------------------
    subroutine t_node_obj_fetch_int32
        type(node) obj
        type(node) n1
        integer res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_fetch_int32")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! Node &my_sub = n.fetch("my_sub");
        ! //or
        ! Node &my_sub = n["my_sub"];
        n1 = obj%fetch("my_sub")
        ! my_sub.set_int32(42)
        call n1%set_int32(42)
        ! n.print_detailed();
        call obj%print_detailed()
        ! int32 res = n.as_int32();
        res = n1%as_int32()
        call assert_equals (42, res)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_fetch_int32

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_fetch_path_int32
        type(node) obj
        type(node) n1
        integer res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_fetch_path_int32")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! n["my_sub"].set_int32(42)
        ! // or
        ! n.set_path_int32("my_sub",42)
        call obj%set_path_int32("my_sub",42)
        ! n.print_detailed()
        call obj%print_detailed()

        ! int32 res = n["my_sub"].as_int32();
        res = obj%fetch_path_as_int32("my_sub")
        call assert_equals(42, res)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_fetch_path_int32
        
    !--------------------------------------------------------------------------
    subroutine t_node_obj_append
        type(node) obj
        type(node) n1
        type(node) n2
        type(node) na
        type(node) nb
        integer(4) res_1
        real(8)    res_2
        integer    nchld

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_append")
        !----------------------------------------------------------------------
        
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n;
        obj = conduit_node_obj_create()
        ! Node &n1 = n.append();
        n1 = obj%append()
        ! Node &n2 = n.append();
        n2 = obj%append()
        
        call assert_true( logical(n2%is_root() .eqv. .false. ))
        ! index_t nchld = n.number_of_children();
        nchld = obj%number_of_children()
        
        call assert_equals(nchld, 2)
        
        ! n1.set_int32(42);
        call n1%set_int32(42)
        ! n1.set_float64(3.1415);
        call n2%set_float64(3.1415d+0)
        
        ! n.print_detailed();
        call obj%print_detailed()
        
        ! Node &na = n[0];
        ! // or
        ! Node &na = n.child(0);
        na  = obj%child(0_8)
        ! Node &nb = n[1];
        ! // or
        ! Node &nb = n.child(1);
        nb  = obj%child(1_8)
        
        call obj%print_detailed()

        !int32 res_1 = n.as_int32();
        res_1 = n1%as_int32()
        !int32 res_2 = n.as_float64();
        res_2 = n2%as_float64()
        
        call assert_equals (42, res_1)
        call assert_equals (3.1415d+0, res_2)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_append
   
    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_fetch_generic
        type(node) obj
        type(node) n
        integer(4) val_int32
        integer(8) val_int64
        real(4)    val_float32
        real(8)    val_float64
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_fetch_generic")
        !----------------------------------------------------------------------
        
        obj = conduit_node_obj_create()
        call obj%set_path("my_int32",42)
        call obj%set_path("my_int64",42_8)
        call obj%set_path("my_float32",3.1415)
        call obj%set_path("my_float64",3.1415d+0)
        call obj%print_detailed()

        val_int32   = obj%fetch_path_as_int32("my_int32")
        val_int64   = obj%fetch_path_as_int64("my_int64")
        val_float32 = obj%fetch_path_as_float32("my_float32")
        val_float64 = obj%fetch_path_as_float64("my_float64")
        call assert_equals(42, val_int32)
        !=======
        ! NOTE:  fruit doesn't support assert_equals with integer(8)
        !=======
        !call assert_equals(42_8, val_int64)
        print *,42_8,"vs",val_int64
        call assert_equals(3.1415, val_float32)
        call assert_equals(3.1415d+0, val_float64)
        call conduit_node_obj_destroy(obj)
        
    end subroutine t_node_obj_set_fetch_generic

   
    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_generic_fetch_ptr
        type(node) n
        type(node) n_den
        real(kind=8), dimension(4) :: den
        real(kind=8), pointer :: d_arr(:)
        character, pointer :: units(:)
        integer i
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_set_generic_fetch_ptr")
        !----------------------------------------------------------------------
        do i = 1,4
            den(i) = 1.0
        enddo
  
        n = conduit_node_obj_create()
        call n%set_path_ptr("fields/density/values",den,4_8)
        call n%set_path("fields/density/units","g/cc")
    
        n_den = n%fetch("fields/density")
  
        call n_den%fetch_path_as_float64_ptr("values",d_arr)
  
        call n_den%fetch_path_as_char8_str("units",units)
  
        do i = 1,4
            call assert_equals(den(i), d_arr(i))
        enddo
  
        call assert_equals(units(1),"g")
        call assert_equals(units(2),"/")
        call assert_equals(units(3),"c")
        call assert_equals(units(4),"c")
        
        call conduit_node_obj_destroy(n)
        
    end subroutine t_node_obj_set_generic_fetch_ptr

    !--------------------------------------------------------------------------
    subroutine t_node_obj_set_node
        type(node) n1
        type(node) n2
        real(kind=8) res
        
        !----------------------------------------------------------------------
        call set_case_name("t_node_set_node")
        !----------------------------------------------------------------------
        n1 = conduit_node_obj_create()
        n2 = conduit_node_obj_create()
        
        call n1%set_path("a",3.1415d+0)
        call n2%set_path("path/to",n1)
        call assert_true( logical(n2%has_path("path/to/a") .eqv. .true.))


        call n2%set_path_external("another/path/to",n1)

        res = n2%fetch_path_as_float64("path/to/a");
        call assert_equals(3.1415d+0, res)

        res = n2%fetch_path_as_float64("another/path/to/a");
        call assert_equals(3.1415d+0, res)

        call n1%set_path_float64("a",42.0d+0)

        res = n2%fetch_path_as_float64("another/path/to/a");
        call assert_equals(42.0d+0, res)
        
        call n2%print()
        call conduit_node_obj_destroy(n1)
        call conduit_node_obj_destroy(n2)

    end subroutine t_node_obj_set_node

    !--------------------------------------------------------------------------
    subroutine t_node_obj_diff
        type(node) obj
        type(node) other
        type(node) info
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_diff")
        !----------------------------------------------------------------------
        
        obj   = conduit_node_obj_create()
        other = conduit_node_obj_create()
        info  = conduit_node_obj_create()

        call obj%set_path("a",42)
        call other%set_path("a",42)

        ! no diff
        call assert_true( logical(obj%diff(other,info,1d-12) .eqv. .false.))
        call other%set_path("b",3.1415d+0)

        ! there is a  diff
        call assert_true( logical(obj%diff(other,info,1d-12) .eqv. .true.))

        ! no compat diff
        call assert_true( logical(obj%diff_compatible(other,info,1d-12) .eqv. .false.))

        call obj%set_path("b",3.1415d+0)

        ! no diff
        call assert_true( logical(obj%diff(other,info,1d-12) .eqv. .false.))

        call conduit_node_obj_destroy(obj)
        call conduit_node_obj_destroy(other)
        call conduit_node_obj_destroy(info)
        
    end subroutine t_node_obj_diff

    !--------------------------------------------------------------------------
    subroutine t_node_obj_update
        type(node) n1
        type(node) n2
        real(kind=8) val
        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_update")
        !----------------------------------------------------------------------
        
        !--------------
        ! c++ ~equiv:
        !--------------
        ! Node n1;
        ! Node n2;
        n1 = conduit_node_obj_create()
        n2 = conduit_node_obj_create()

        ! n1["a"].set_float64(3.1415);
        call n1%set_path("a",3.1415d+0)
        ! n2.update(n1)
        call n2%update(n1)

        call assert_true( logical(n2%has_path("a") .eqv. .true.))

        call n1%set_path("a",42.0d+0)
        call n1%set_path("b",52.0d+0)

        ! n2.update_compatible(n1)
        call n2%update_compatible(n1)
        ! float64 val = n2["a"].value()
        val = n2%fetch_path_as_float64("a");

        call assert_equals(42.0d+0, val)

        call assert_true( logical(n2%has_path("a") .eqv. .true.))
        call assert_true( logical(n2%has_path("b") .eqv. .false.))


        ! n2.update_external(n1)
        call n2%update_external(n1)
        ! n2["a"].set(float64(62.0));
        call n1%set_path("a",62d+0)
        ! float64 val = n2["a"].value()
        val = n2%fetch_path_as_float64("a");

        call assert_equals(62.0d+0, val)

        call assert_true( logical(n2%has_path("a") .eqv. .true.))
        call assert_true( logical(n2%has_path("a") .eqv. .true.))

        val = n2%fetch_path_as_float64("b");

        call assert_equals(52.0d+0, val)

        call conduit_node_obj_destroy(n1)
        call conduit_node_obj_destroy(n2)

    end subroutine t_node_obj_update
    
    !--------------------------------------------------------------------------
    subroutine t_node_obj_compact_to
        type(node) n1
        type(node) n2
        real(kind=8) val
        integer    bytes_res

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_compact_to")
        !----------------------------------------------------------------------
        
        n1 = conduit_node_obj_create()
        n2 = conduit_node_obj_create()

        call n1%set_path("a",10)
        call n1%set_path("b",20)
        call n1%set_path("c",30d+0)

        bytes_res = n1%total_bytes_allocated();
        
        call assert_equals( bytes_res, 16)

        call assert_true( logical(n1%is_contiguous() .eqv. .false.))
        
        call n1%compact_to(n2);

        call assert_true( logical(n2%is_contiguous() .eqv. .true.))

        bytes_res = n2%total_bytes_compact();
        call assert_equals( bytes_res, 16)
        bytes_res = n2%total_strided_bytes()
        call assert_equals( bytes_res, 16)
        bytes_res = n2%total_bytes_allocated()
        call assert_equals( bytes_res, 16)


        call conduit_node_obj_destroy(n1)
        call conduit_node_obj_destroy(n2)


    end subroutine t_node_obj_compact_to
    
    
    !--------------------------------------------------------------------------
    subroutine t_node_obj_remove
        type(node) n
        real(kind=8) val

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_remove")
        !----------------------------------------------------------------------

        n = conduit_node_obj_create()

        call n%set_path("a",62d+0)
        call assert_true( logical(n%has_path("a") .eqv. .true.))
        call n%remove_path("a")
        call assert_true( logical(n%has_path("a") .eqv. .false.))

        call n%set_path("a",62d+0)
        call assert_true( logical(n%has_path("a") .eqv. .true.))
        ! remove child using idx (still using zero-based idx)
        call n%remove_child(0_8)
        call assert_true( logical(n%has_path("a") .eqv. .false.))


        call n%set_path("a",62d+0)
        call n%rename_child("a","b")
        call assert_true( logical(n%has_path("a") .eqv. .false.))
        call assert_true( logical(n%has_path("b") .eqv. .true.))

        val = n%fetch_path_as_float64("b");

        call assert_equals(62.0d+0, val)

        call conduit_node_obj_destroy(n)

    end subroutine t_node_obj_remove

   !--------------------------------------------------------------------------
    subroutine t_node_obj_parse
        type(node) n
        real(kind=8) val

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_parse")
        !----------------------------------------------------------------------

        n = conduit_node_obj_create()

        call n%parse('{"a": 42.0}',"json")
        val = n%fetch_path_as_float64("a");

        call assert_equals(42.0d+0, val)

        call n%parse("a: 42.0","yaml")
        val = n%fetch_path_as_float64("a");

        call conduit_node_obj_destroy(n)

    end subroutine t_node_obj_parse
 
    !--------------------------------------------------------------------------
    subroutine t_node_obj_save_load
        type(node) n1
        type(node) n2
        real(kind=8) val

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_parse")
        !----------------------------------------------------------------------

        n1 = conduit_node_obj_create()
        n2 = conduit_node_obj_create()
        call n1%set_path("a",42d+0)

        call n1%save("tout_f_node_obj_save.json","json")
        call n2%load("tout_f_node_obj_save.json","json")
        val = n2%fetch_path_as_float64("a");
        call assert_equals(42.0d+0, val)

        call n1%save("tout_f_node_obj_save.yaml","yaml")
        call n2%load("tout_f_node_obj_save.yaml","yaml")
        val = n2%fetch_path_as_float64("a");
        call assert_equals(42.0d+0, val)

        call conduit_node_obj_destroy(n1)
        call conduit_node_obj_destroy(n2)

    end subroutine t_node_obj_save_load
   !--------------------------------------------------------------------------
    subroutine t_node_obj_names_embedded_slashes
        type(node) n
        type(node) n_1
        type(node) n_2
        type(node) n_2_test
        real(kind=8) val
        integer      nchld

        !----------------------------------------------------------------------
        call set_case_name("t_node_obj_names_embedded_slashes")
        !----------------------------------------------------------------------

        n = conduit_node_obj_create()

        n_1 = n%fetch("normal/path")
        n_2 = n%add_child("child_with_/_inside")

        call n_1%set(10.0d+0)
        call n_2%set(42.0d+0)

        val = n_1%as_float64();
        call assert_equals(10.0d+0, val)

        val = n_2%as_float64();
        call assert_equals(42.0d+0, val)

        call assert_true( logical(n%has_path("normal/path") .eqv. .true.))
        call assert_true( logical(n%has_child("normal/path") .eqv. .false.))

        call assert_true( logical(n%has_path("child_with_/_inside") .eqv. .false.))
        call assert_true( logical(n%has_child("child_with_/_inside") .eqv. .true.))

        nchld = n%number_of_children()
        call assert_equals( nchld, 2 )

        n_2_test = n%child("child_with_/_inside")
        val = n_2_test%as_float64()
        call assert_equals(42.0d+0, val)

        call n%remove_child("child_with_/_inside")

        nchld = n%number_of_children()
        call assert_equals( nchld, 1 )
        call assert_true( logical(n%has_path("normal/path") .eqv. .true.))

        call conduit_node_obj_destroy(n)

    end subroutine t_node_obj_names_embedded_slashes
    
    !--------------------------------------------------------------------------
     subroutine t_node_obj_fetch_existing
         type(node) n
         type(node) n_1
         type(node) n_1_test
         real(kind=8) val


         !----------------------------------------------------------------------
         call set_case_name("t_node_obj_fetch_existing")
         !----------------------------------------------------------------------

         n = conduit_node_obj_create()

         n_1 = n%fetch("normal/path")
         call n_1%set(10.0d+0)
         val = n_1%as_float64();
         call assert_equals(10.0d+0, val)

         n_1_test = n%fetch_existing("normal/path")

         val = n_1_test%as_float64();
         call assert_equals(10.0d+0, val)
    
         call conduit_node_obj_destroy(n)

     end subroutine t_node_obj_fetch_existing

     !--------------------------------------------------------------------------
     subroutine t_node_obj_reset
         type(node) n
         type(node) n_1
         integer     nchld
         integer     res

         !----------------------------------------------------------------------
         call set_case_name("t_node_obj_reset")
         !----------------------------------------------------------------------

         !--------------
         ! c++ ~equiv:
         !--------------
         ! Node n;    
         n = conduit_node_obj_create()
         ! Node &n_1 = n["normal/path"];
         n_1 = n%fetch("normal/path")
         nchld = n%number_of_children()
         call assert_equals(nchld, 1)
         ! n.reset()
         call n%reset()
         nchld = n%number_of_children()
         call assert_equals(nchld, 0)
         call conduit_node_obj_destroy(n)

     end subroutine t_node_obj_reset

     !--------------------------------------------------------------------------
     subroutine t_node_obj_move_and_swap
         type(node) n_a
         type(node) n_b
         integer     nchld
         integer    res

         !----------------------------------------------------------------------
         call set_case_name("t_node_obj_move_and_swap")
         !----------------------------------------------------------------------

         !--------------
         ! c++ ~equiv:
         !--------------
         ! Node n_a;
         ! n_a["data"] = 10;
         n_a = conduit_node_obj_create()
         call n_a%set_path_int32("data",10)
         ! Node n_b;
         ! n_b["data"] = 20;
         n_b = conduit_node_obj_create()
         call n_b%set_path_int32("data",20)

         ! n_a.swap(n_b);
         call n_a%swap(n_b)

         ! check they are swapped
         res = n_b%fetch_path_as_int32("data")
         call assert_equals(10, res)

         res = n_a%fetch_path_as_int32("data")
         call assert_equals(20, res)

         ! n_b.swap(n_a);
         ! now move b into a, b will be reset as a result
         call n_a%move(n_b)

         ! b should be empty
         nchld = n_b%number_of_children()
         call assert_equals(0, nchld)

         ! back to where we started with n_a
         res = n_a%fetch_path_as_int32("data")
         call assert_equals(10, res)

         call conduit_node_obj_destroy(n_a)
         call conduit_node_obj_destroy(n_b)

     end subroutine t_node_obj_move_and_swap


!------------------------------------------------------------------------------
end module f_conduit_node_obj
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
program fortran_test
!------------------------------------------------------------------------------
  use fruit
  use f_conduit_node_obj
  implicit none
  logical ok
  call init_fruit
  !----------------------------------------------------------------------------
  ! call our test routines
  !----------------------------------------------------------------------------
  call t_node_obj_create
  call t_node_obj_set_int
  call t_node_obj_set_int32
  call t_node_obj_set_double
  call t_node_obj_set_float64
  call t_node_obj_set_and_fetch_path_int32_ptr
  call t_node_obj_set_and_fetch_path_int64_ptr
  call t_node_obj_set_and_fetch_path_float32_ptr
  call t_node_obj_set_and_fetch_path_float64_ptr
  call t_node_obj_fetch_int32
  call t_node_obj_set_int32_ptr
  call t_node_obj_set_external_int32_ptr
  call t_node_obj_as_int32_ptr
  call t_node_obj_as_int32_ptr_read_scalar
  call t_node_obj_append
  call t_node_obj_set_fetch_path_int32
  call t_node_obj_set_fetch_generic
  call t_node_obj_set_generic_fetch_ptr
  call t_node_obj_set_node
  call t_node_obj_diff
  call t_node_obj_update
  call t_node_obj_compact_to
  call t_node_obj_remove
  call t_node_obj_parse
  call t_node_obj_save_load
  call t_node_obj_names_embedded_slashes
  call t_node_obj_fetch_existing
  call t_node_obj_reset
  call t_node_obj_move_and_swap

  call fruit_summary
  call fruit_finalize
  call is_all_successful(ok)
  
  if (.not. ok) then
     call exit(1)
  endif

!------------------------------------------------------------------------------
end program fortran_test
!------------------------------------------------------------------------------
