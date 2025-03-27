!* Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
!* Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
!* other details. No copyright assignment is required to contribute to Conduit.

!------------------------------------------------------------------------------
!
! t_f_conduit_node_datatype.f
!
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
module t_f_conduit_node_datatype
!------------------------------------------------------------------------------

  use iso_c_binding
  use fruit
  use conduit
  implicit none

!------------------------------------------------------------------------------
contains
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------    
!  Opaque Pointer Function Style test
!------------------------------------------------------------------------------
      subroutine t_node_datatype_ids
          type(C_PTR) cnode_a
          type(C_PTR) list
          type(C_PTR) dataType
          integer, parameter :: int64 = selected_int_kind(15)
          integer(4) i
          integer(C_INT) :: cint  = 42
          real(C_DOUBLE) :: cdouble = 42.0
        
          
          !----------------------------------------------------------------------
          call set_case_name("t_node_datatype_ids")
          !----------------------------------------------------------------------

          !--------------
          ! c++ ~equiv:
          !--------------
          ! Node n_a;
          cnode_a = conduit_node_create()
          dataType = conduit_node_dtype(cnode_a)
          
          call assert_true( c_conduit_datatype_is_empty(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_EMPTY_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 0)
          call assert_true( conduit_datatype_stride(dataType) == 0)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 0)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! n_a["data"] = 42;
          call conduit_node_set_path_int32(cnode_a,"data",42)
          call assert_true( c_conduit_datatype_is_object(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_OBJECT_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 0)
          call assert_true( conduit_datatype_stride(dataType) == 0)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 0)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! Node n_a 
          ! for(int i = 0; i < 5 ; i++ )
          ! {
          !    Node &list_entry = n.append();
          !    list_entry.set(i);
          ! }
          do i = 1,5
            list = conduit_node_append(cnode_a)
            call conduit_node_set_int32(list,i)
          enddo

          call assert_true( c_conduit_datatype_is_list(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_LIST_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 0)
          call assert_true( conduit_datatype_stride(dataType) == 0)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 0)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
           
          ! Node n_a = int32(-42)
          call conduit_node_set_int32(cnode_a,-42)
          call assert_true( c_conduit_datatype_is_int32(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_INT32_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 4)
          call assert_true( conduit_datatype_stride(dataType) == 4)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 1)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 1)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 1)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          
          ! Node n_a = int64(-42)
          call conduit_node_set_int64(cnode_a,42_int64)
          call assert_true( c_conduit_datatype_is_int64(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_INT64_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 8)
          call assert_true( conduit_datatype_stride(dataType) == 8)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 1)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 1)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 1)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! Node n_a = float(42.0)
          call conduit_node_set_float32(cnode_a,42.0)
          call assert_true( c_conduit_datatype_is_float32(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_FLOAT32_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 4)
          call assert_true( conduit_datatype_stride(dataType) == 4)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 1)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 1)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! Node n_a = double(42.0)
          call conduit_node_set_float64(cnode_a,3.1415d+0)
          call assert_true( c_conduit_datatype_is_float64(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_FLOAT64_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 8)
          call assert_true( conduit_datatype_stride(dataType) == 8)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          
          ! Test the new element stride methods
          call assert_true( conduit_datatype_element_stride(dataType) == 1) ! 8/8 = 1
          call assert_true( c_conduit_datatype_is_stride_element_aligned(dataType) == 1) ! 8 % 8 = 0
          call assert_true( c_conduit_datatype_is_stride_aligned(dataType, 8) == 1) ! 8 % 8 = 0
          call assert_true( c_conduit_datatype_is_stride_aligned(dataType, 4) == 1) ! 8 % 4 = 0
          call assert_true( c_conduit_datatype_is_stride_aligned(dataType, 2) == 1) ! 8 % 2 = 0
          
          call assert_true( c_conduit_datatype_is_number(dataType) == 1)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 1)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! Node n_a = "test string"
          call conduit_node_set_char8_str(cnode_a,"test string")
          call assert_true( c_conduit_datatype_is_char8_str(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_CHAR8_STR_ID)
          call assert_true( conduit_datatype_element_bytes(dataType) == 1)
          call assert_true( conduit_datatype_stride(dataType) == 1)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 0)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true( c_conduit_datatype_is_string(dataType) == 1)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! Node n_a = cint
          call conduit_node_set_int(cnode_a,cint)
          call assert_true( c_conduit_datatype_is_int(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_INT_ID)
          ! depends on type of int in C
          ! call assert_true( conduit_datatype_element_bytes(dataType) == 1)
          !call assert_true( conduit_datatype_stride(dataType) == 1)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 1)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 1)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 1)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 0)
          call assert_true( c_conduit_datatype_is_string(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          ! Node n_a = double(42)
          call conduit_node_set_double(cnode_a,cdouble)
          call assert_true( c_conduit_datatype_is_double(dataType) == 1)
          call assert_true( conduit_datatype_id(dataType) == CONDUIT_DOUBLE_ID)
          
          call assert_true( conduit_datatype_element_bytes(dataType) == 8)
          call assert_true( conduit_datatype_stride(dataType) == 8)
          call assert_true( conduit_datatype_offset(dataType) == 0)
          call assert_true( c_conduit_datatype_is_number(dataType) == 1)
          call assert_true( c_conduit_datatype_is_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_signed_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_unsigned_integer(dataType) == 0)
          call assert_true( c_conduit_datatype_is_floating_point(dataType) == 1)
          call assert_true( c_conduit_datatype_is_string(dataType) == 0)
          call assert_true(c_conduit_datatype_endianness_matches_machine(dataType) == 1)
          
          
          call conduit_node_destroy(cnode_a)
          ! TODO pick 4/8 on compile time
          !call assert_equals(conduit_datatype_sizeof_index_t(),4)
          !call assert_equals(conduit_datatype_sizeof_index_t(),8)

      end subroutine t_node_datatype_ids

!------------------------------------------------------------------------------
end module t_f_conduit_node_datatype
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
program fortran_test
!------------------------------------------------------------------------------
  use fruit
  use t_f_conduit_node_datatype
  implicit none
  logical ok
  
  call init_fruit

  !----------------------------------------------------------------------------
  ! call our test routines
  !----------------------------------------------------------------------------
  call t_node_datatype_ids

  call fruit_summary
  call fruit_finalize
  call is_all_successful(ok)
  
  if (.not. ok) then
     call exit(1)
  endif

!------------------------------------------------------------------------------
end program fortran_test
!------------------------------------------------------------------------------


