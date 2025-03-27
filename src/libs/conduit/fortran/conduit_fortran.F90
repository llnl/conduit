!* Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
!* Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
!* other details. No copyright assignment is required to contribute to Conduit.

!------------------------------------------------------------------------------
! conduit_fortran.f
!------------------------------------------------------------------------------

#include "conduit_fortran_bitwidth_style_types.inc"

!------------------------------------------------------------------------------
module conduit
!------------------------------------------------------------------------------
    use, intrinsic :: iso_c_binding, only : C_PTR
    implicit none

    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! Conduit DataType IDs
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! generic types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_EMPTY_ID  = F_CONDUIT_EMPTY_ID
    integer, parameter :: CONDUIT_OBJECT_ID = F_CONDUIT_OBJECT_ID
    integer, parameter :: CONDUIT_LIST_ID   = F_CONDUIT_LIST_ID
    
    !--------------------------------------------------------------------------
    ! bitwidth style signed integer types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_INT8_ID   = F_CONDUIT_INT8_ID
    integer, parameter :: CONDUIT_INT16_ID  = F_CONDUIT_INT16_ID
    integer, parameter :: CONDUIT_INT32_ID  = F_CONDUIT_INT32_ID
    integer, parameter :: CONDUIT_INT64_ID  = F_CONDUIT_INT64_ID
    !--------------------------------------------------------------------------
    ! bitwidth style unsigned integer types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_UINT8_ID   = F_CONDUIT_UINT8_ID
    integer, parameter :: CONDUIT_UINT16_ID  = F_CONDUIT_UINT16_ID
    integer, parameter :: CONDUIT_UINT32_ID  = F_CONDUIT_UINT32_ID
    integer, parameter :: CONDUIT_UINT64_ID  = F_CONDUIT_UINT64_ID
    !--------------------------------------------------------------------------
    ! bitwidth style floating point integer types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_FLOAT32_ID  = F_CONDUIT_FLOAT32_ID
    integer, parameter :: CONDUIT_FLOAT64_ID  = F_CONDUIT_FLOAT64_ID
    !--------------------------------------------------------------------------
    ! string  types 
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_CHAR8_STR_ID = F_CONDUIT_CHAR8_STR_ID

    !--------------------------------------------------------------------------
    ! c style type ids
    !--------------------------------------------------------------------------
    ! these are mapped at configure time to the proper bitwidth style type ids.
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! c style signed integer types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_CHAR_ID  = F_CONDUIT_NATIVE_SIGNED_CHAR_ID
    integer, parameter :: CONDUIT_SHORT_ID = F_CONDUIT_NATIVE_SIGNED_SHORT_ID
    integer, parameter :: CONDUIT_INT_ID   = F_CONDUIT_NATIVE_SIGNED_INT_ID
    integer, parameter :: CONDUIT_LONG_ID  = F_CONDUIT_NATIVE_SIGNED_LONG_ID
    !--------------------------------------------------------------------------
    ! c style unsigned integer types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_UCHAR_ID  = F_CONDUIT_NATIVE_UNSIGNED_CHAR_ID
    integer, parameter :: CONDUIT_USHORT_ID = F_CONDUIT_NATIVE_UNSIGNED_SHORT_ID
    integer, parameter :: CONDUIT_UINT_ID   = F_CONDUIT_NATIVE_UNSIGNED_INT_ID
    integer, parameter :: CONDUIT_ULONG_ID  = F_CONDUIT_NATIVE_UNSIGNED_LONG_ID
    !--------------------------------------------------------------------------
    ! c style floating point integer types
    !--------------------------------------------------------------------------
    integer, parameter :: CONDUIT_FLOAT_ID  = F_CONDUIT_NATIVE_FLOAT_ID
    integer, parameter :: CONDUIT_DOUBLE_ID = F_CONDUIT_NATIVE_DOUBLE_ID
    !--------------------------------------------------------------------------

    ! conduit_index_t as defined in c
    integer, parameter :: CONDUIT_INDEX_ID = F_CONDUIT_INDEX_ID


    !--------------------------------------------------------------------------
    interface
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! about
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_about(cnode) &
             bind(C, name="conduit_about")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
     end subroutine conduit_about

    !--------------------------------------------------------------------------
    ! construction and destruction
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    function conduit_node_create() result(cnode) &
             bind(C, name="conduit_node_create")
         use iso_c_binding
         implicit none
         type(C_PTR) :: cnode
     end function conduit_node_create

    !--------------------------------------------------------------------------
    subroutine conduit_node_destroy(cnode) &
            bind(C, name="conduit_node_destroy")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
    end subroutine conduit_node_destroy


    !--------------------------------------------------------------------------
    ! object and list interface
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    function c_conduit_node_fetch(cnode, path) result(res) &
             bind(C, name="conduit_node_fetch")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR) :: res
     end function c_conduit_node_fetch

     !--------------------------------------------------------------------------
     function c_conduit_node_fetch_existing(cnode, path) result(res) &
              bind(C, name="conduit_node_fetch_existing")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          character(kind=C_CHAR), intent(IN) :: path(*)
          type(C_PTR) :: res
      end function c_conduit_node_fetch_existing

     !--------------------------------------------------------------------------
     function conduit_node_append(cnode) result(res) &
              bind(C, name="conduit_node_append")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          type(C_PTR) :: res
      end function conduit_node_append

      !--------------------------------------------------------------------------
      function conduit_node_child(cnode,idx) result(res) &
               bind(C, name="conduit_node_child")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           integer(C_SIZE_T), value, intent(in) :: idx
           type(C_PTR) :: res
       end function conduit_node_child

    !--------------------------------------------------------------------------
    function c_conduit_node_child_by_name(cnode,name) result(res) &
            bind(C, name="conduit_node_child_by_name")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: name(*)
        type(C_PTR) :: res
    end function c_conduit_node_child_by_name

    !--------------------------------------------------------------------------
    ! node info methods
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    function c_conduit_node_is_root(cnode) result(res) &
             bind(C, name="conduit_node_is_root")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(C_INT) :: res
     end function c_conduit_node_is_root

     !--------------------------------------------------------------------------
     function c_conduit_node_is_data_external(cnode) result(res) &
           bind(C, name="conduit_node_is_data_external")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(C_INT) :: res
     end function c_conduit_node_is_data_external

    !--------------------------------------------------------------------------
    function conduit_node_parent(cnode) result(res) &
           bind(C, name="conduit_node_parent")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        type(C_PTR) :: res
    end function conduit_node_parent

    !--------------------------------------------------------------------------
    function conduit_node_number_of_elements(cnode) result(res) &
            bind(C, name="conduit_node_number_of_elements")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(C_SIZE_T) :: res
    end function conduit_node_number_of_elements

    !--------------------------------------------------------------------------
    function conduit_node_number_of_children(cnode) result(res) &
             bind(C, name="conduit_node_number_of_children")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(C_SIZE_T) :: res
     end function conduit_node_number_of_children
     
     !--------------------------------------------------------------------------
     function c_conduit_node_has_child(cnode,name) result(res) &
              bind(C, name="conduit_node_has_child")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          character(kind=C_CHAR), intent(IN) :: name(*)
          integer(C_INT) :: res
      end function c_conduit_node_has_child
     
      !--------------------------------------------------------------------------
      subroutine c_conduit_node_remove_path(cnode,path) &
               bind(C, name="conduit_node_remove_path")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           character(kind=C_CHAR), intent(IN) :: path(*)
      end subroutine c_conduit_node_remove_path

      !--------------------------------------------------------------------------
      subroutine conduit_node_remove_child(cnode,idx) &
               bind(C, name="conduit_node_remove_child")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           integer(C_SIZE_T), value, intent(in) :: idx
      end subroutine conduit_node_remove_child

      !--------------------------------------------------------------------------
      function c_conduit_node_add_child(cnode,name) result(res) &
               bind(C, name="conduit_node_add_child")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           character(kind=C_CHAR), intent(IN) :: name(*)
           type(C_PTR) :: res
      end function c_conduit_node_add_child

      !--------------------------------------------------------------------------
      subroutine c_conduit_node_remove_child_by_name(cnode,name) &
               bind(C, name="conduit_node_remove_child_by_name")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           character(kind=C_CHAR), intent(IN) :: name(*)
      end subroutine c_conduit_node_remove_child_by_name

      !--------------------------------------------------------------------------
      subroutine c_conduit_node_rename_child(cnode,old_name, new_name) &
                bind(C, name="conduit_node_rename_child")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode
            character(kind=C_CHAR), intent(IN) :: old_name(*)
            character(kind=C_CHAR), intent(IN) :: new_name(*)
      end subroutine c_conduit_node_rename_child
      !--------------------------------------------------------------------------
      function c_conduit_node_has_path(cnode,path) result(res) &
               bind(C, name="conduit_node_has_path")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           character(kind=C_CHAR), intent(IN) :: path(*)
           integer(C_INT) :: res
       end function c_conduit_node_has_path

       !--------------------------------------------------------------------------
       function conduit_node_total_strided_bytes(cnode) result(res) &
               bind(C, name="conduit_node_total_strided_bytes")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           integer(C_SIZE_T) :: res
       end function conduit_node_total_strided_bytes

       !--------------------------------------------------------------------------
       function conduit_node_total_bytes_allocated(cnode) result(res) &
               bind(C, name="conduit_node_total_bytes_allocated")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           integer(C_SIZE_T) :: res
       end function conduit_node_total_bytes_allocated

       !--------------------------------------------------------------------------
       function conduit_node_total_bytes_compact(cnode) result(res) &
               bind(C, name="conduit_node_total_bytes_compact")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           integer(C_SIZE_T) :: res
       end function conduit_node_total_bytes_compact

       !--------------------------------------------------------------------------
       function c_conduit_node_is_compact(cnode) result(res) &
                bind(C, name="conduit_node_is_compact")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode
            integer(C_INT) :: res
        end function c_conduit_node_is_compact
    
        !--------------------------------------------------------------------------
        function c_conduit_node_is_contiguous(cnode) result(res) &
             bind(C, name="conduit_node_is_contiguous")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(C_INT) :: res
        end function c_conduit_node_is_contiguous

        !--------------------------------------------------------------------------
        function c_conduit_node_contiguous_with_node(cnode,cother) result(res) &
              bind(C, name="conduit_node_contiguous_with_node")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          type(C_PTR), value, intent(IN) :: cother
          integer(C_INT) :: res
        end function c_conduit_node_contiguous_with_node

        !--------------------------------------------------------------------------
        subroutine conduit_node_compact_to(cnode,cdest) &
                bind(C, name="conduit_node_compact_to")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode
            type(C_PTR), value, intent(IN) :: cdest
         end subroutine conduit_node_compact_to

        !--------------------------------------------------------------------------
        function c_conduit_node_diff(cnode,cother,cinfo,epsilon) result(res) &
              bind(C, name="conduit_node_diff")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          type(C_PTR), value, intent(IN) :: cother
          type(C_PTR), value, intent(IN) :: cinfo
          real(8), value, intent(IN) :: epsilon
          integer(C_INT) :: res
        end function c_conduit_node_diff

        !--------------------------------------------------------------------------
        function c_conduit_node_diff_compatible(cnode,cother,cinfo,epsilon) result(res) &
              bind(C, name="conduit_node_diff_compatible")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          type(C_PTR), value, intent(IN) :: cother
          type(C_PTR), value, intent(IN) :: cinfo
          real(8), value, intent(IN) :: epsilon
          integer(C_INT) :: res
        end function c_conduit_node_diff_compatible

       !--------------------------------------------------------------------------
       function c_conduit_node_compatible(cnode,cother) result(res) &
               bind(C, name="conduit_node_compatible")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           type(C_PTR), value, intent(IN) :: cother
           integer(C_INT) :: res
        end function c_conduit_node_compatible

        !--------------------------------------------------------------------------
        subroutine conduit_node_info(cnode,cdest) &
                bind(C, name="conduit_node_info")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode
            type(C_PTR), value, intent(IN) :: cdest
         end subroutine conduit_node_info

        !--------------------------------------------------------------------------
        subroutine conduit_node_reset(cnode) &
                 bind(C, name="conduit_node_reset")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode
        end subroutine conduit_node_reset

        !--------------------------------------------------------------------------
        subroutine conduit_node_move(cnode_a, cnode_b) &
                 bind(C, name="conduit_node_move")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode_a
            type(C_PTR), value, intent(IN) :: cnode_b
        end subroutine conduit_node_move

        !--------------------------------------------------------------------------
        subroutine conduit_node_swap(cnode_a, cnode_b) &
                 bind(C, name="conduit_node_swap")
            use iso_c_binding
            implicit none
            type(C_PTR), value, intent(IN) :: cnode_a
            type(C_PTR), value, intent(IN) :: cnode_b
        end subroutine conduit_node_swap

     !--------------------------------------------------------------------------
     ! node update methods
     !--------------------------------------------------------------------------

     !--------------------------------------------------------------------------
     subroutine conduit_node_update(cnode,cdest) &
             bind(C, name="conduit_node_update")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR), value, intent(IN) :: cdest
      end subroutine conduit_node_update

      !--------------------------------------------------------------------------
      subroutine conduit_node_update_compatible(cnode,cdest) &
              bind(C, name="conduit_node_update_compatible")
          use iso_c_binding
          implicit none
          type(C_PTR), value, intent(IN) :: cnode
          type(C_PTR), value, intent(IN) :: cdest
       end subroutine conduit_node_update_compatible

       !--------------------------------------------------------------------------
       subroutine conduit_node_update_external(cnode,cdest) &
               bind(C, name="conduit_node_update_external")
           use iso_c_binding
           implicit none
           type(C_PTR), value, intent(IN) :: cnode
           type(C_PTR), value, intent(IN) :: cdest
        end subroutine conduit_node_update_external

    !--------------------------------------------------------------------------
    ! -- basic io, parsing, and generation ---
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    subroutine c_conduit_node_parse(cnode, schema, protocol) &
        bind(C, name="conduit_node_parse")
    use iso_c_binding
    implicit none
    type(C_PTR), value, intent(IN) :: cnode
    character(kind=C_CHAR), intent(IN) :: schema(*)
    character(kind=C_CHAR), intent(IN) :: protocol(*)
    end subroutine c_conduit_node_parse

    !--------------------------------------------------------------------------
    subroutine c_conduit_node_load(cnode, path, protocol) &
        bind(C, name="conduit_node_load")
    use iso_c_binding
    implicit none
    type(C_PTR), value, intent(IN) :: cnode
    character(kind=C_CHAR), intent(IN) :: path(*)
    character(kind=C_CHAR), intent(IN) :: protocol(*)
    end subroutine c_conduit_node_load

    !--------------------------------------------------------------------------
    subroutine c_conduit_node_save(cnode, path, protocol) &
        bind(C, name="conduit_node_save")
    use iso_c_binding
    implicit none
    type(C_PTR), value, intent(IN) :: cnode
    character(kind=C_CHAR), intent(IN) :: path(*)
    character(kind=C_CHAR), intent(IN) :: protocol(*)
    end subroutine c_conduit_node_save

     !--------------------------------------------------------------------------
     ! node print helpers
     !--------------------------------------------------------------------------

     !--------------------------------------------------------------------------
     subroutine conduit_node_print(cnode) &
         bind(C, name="conduit_node_print")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
     end subroutine conduit_node_print

     !--------------------------------------------------------------------------
     subroutine conduit_node_print_detailed(cnode) &
         bind(C, name="conduit_node_print_detailed")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
     end subroutine conduit_node_print_detailed

     !--------------------------------------------------------------------------
     !--------------------------------------------------------------------------
     !--------------------------------------------------------------------------
     ! BEGIN set with node methods
     !--------------------------------------------------------------------------
     !--------------------------------------------------------------------------
     !--------------------------------------------------------------------------

     !--------------------------------------------------------------------------
     ! set_node
     !--------------------------------------------------------------------------
     subroutine conduit_node_set_node(cnode, cother) &
                    bind(C, name="conduit_node_set_node")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR), value, intent(IN) :: cother
     end subroutine conduit_node_set_node

     !--------------------------------------------------------------------------
     ! set_node_external
     !--------------------------------------------------------------------------
     subroutine conduit_node_set_external_node(cnode, cother) &
                    bind(C, name="conduit_node_set_external_node")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR), value, intent(IN) :: cother
     end subroutine conduit_node_set_external_node

     !--------------------------------------------------------------------------
     ! set_node_path
     !--------------------------------------------------------------------------
     subroutine c_conduit_node_set_path_node(cnode, path, cother) &
                    bind(C, name="conduit_node_set_path_node")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR), value, intent(IN) :: cother
     end subroutine c_conduit_node_set_path_node

     !--------------------------------------------------------------------------
     ! set_node_path_external
     !--------------------------------------------------------------------------
     subroutine c_conduit_node_set_path_external_node(cnode, path, cother) &
                    bind(C, name="conduit_node_set_path_external_node")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR), value, intent(IN) :: cother
     end subroutine c_conduit_node_set_path_external_node

    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! BEGIN int32 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! int32 set
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_int32(cnode, val) &
                   bind(C, name="conduit_node_set_int32")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(4), value, intent(IN) :: val
    end subroutine conduit_node_set_int32

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_int32_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_int32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_int32_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_external_int32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_external_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_int32_ptr_detailed(cnode, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_external_int32_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine conduit_node_set_external_int32_ptr_detailed

    !--------------------------------------------------------------------------
    ! int32 set_path
    !--------------------------------------------------------------------------

    subroutine c_conduit_node_set_path_int32(cnode, path, val) &
                   bind(C, name="conduit_node_set_path_int32")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(4), value, intent(IN) :: val
    end subroutine c_conduit_node_set_path_int32


    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_int32_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_int32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_int32_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_int32_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_external_int32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_external_int32_ptr

    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_int32_ptr_detailed(cnode, path, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_path_external_int32_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine c_conduit_node_set_path_external_int32_ptr_detailed


    !--------------------------------------------------------------------------
    ! int32 as
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    pure function conduit_node_as_int32(cnode) result(res) &
             bind(C, name="conduit_node_as_int32")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(4) :: res
    end function conduit_node_as_int32

    !--------------------------------------------------------------------------
    function c_conduit_node_as_int32_ptr(cnode) result(int32_ptr) &
             bind(C, name="conduit_node_as_int32_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR) :: int32_ptr
     end function c_conduit_node_as_int32_ptr

    !--------------------------------------------------------------------------
    pure function c_conduit_node_fetch_path_as_int32(cnode, path) result(res) &
                   bind(C, name="conduit_node_fetch_path_as_int32")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(4) :: res
    end function c_conduit_node_fetch_path_as_int32

    !--------------------------------------------------------------------------
    function c_conduit_node_fetch_path_as_int32_ptr(cnode, path) result(int32_ptr) &
             bind(C, name="conduit_node_fetch_path_as_int32_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR) :: int32_ptr
     end function c_conduit_node_fetch_path_as_int32_ptr
     


    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! END int32 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! BEGIN int64 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! int64 set
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_int64(cnode, val) &
                   bind(C, name="conduit_node_set_int64")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(8), value, intent(IN) :: val
    end subroutine conduit_node_set_int64

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_int64_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_int64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_int64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_int64_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_external_int64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_external_int64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_int64_ptr_detailed(cnode, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_external_int64_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine conduit_node_set_external_int64_ptr_detailed

    !--------------------------------------------------------------------------
    ! int64 set_path
    !--------------------------------------------------------------------------

    subroutine c_conduit_node_set_path_int64(cnode, path, val) &
                   bind(C, name="conduit_node_set_path_int64")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(8), value, intent(IN) :: val
    end subroutine c_conduit_node_set_path_int64


    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_int64_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_int64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_int64_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_int64_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_external_int64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_external_int64_ptr
    

    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_int64_ptr_detailed(cnode, path, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_path_external_int64_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine c_conduit_node_set_path_external_int64_ptr_detailed
    !--------------------------------------------------------------------------
    ! int64 as
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    pure function conduit_node_as_int64(cnode) result(res) &
             bind(C, name="conduit_node_as_int64")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(8) :: res
    end function conduit_node_as_int64

    !--------------------------------------------------------------------------
    function c_conduit_node_as_int64_ptr(cnode) result(int64_ptr) &
             bind(C, name="conduit_node_as_int64_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR) :: int64_ptr
     end function c_conduit_node_as_int64_ptr

    !--------------------------------------------------------------------------
    pure function c_conduit_node_fetch_path_as_int64(cnode, path) result(res) &
                   bind(C, name="conduit_node_fetch_path_as_int64")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        integer(8) :: res
    end function c_conduit_node_fetch_path_as_int64

    !--------------------------------------------------------------------------
    function c_conduit_node_fetch_path_as_int64_ptr(cnode, path) result(int64_ptr) &
             bind(C, name="conduit_node_fetch_path_as_int64_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR) :: int64_ptr
     end function c_conduit_node_fetch_path_as_int64_ptr
     


    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! END int64 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------


    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! BEGIN float32 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! float32 set
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_float32(cnode, val) &
                   bind(C, name="conduit_node_set_float32")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(4), value, intent(IN) :: val
    end subroutine conduit_node_set_float32

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_float32_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_float32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_float32_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_external_float32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_external_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_float32_ptr_detailed(cnode, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_external_float32_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine conduit_node_set_external_float32_ptr_detailed

    !--------------------------------------------------------------------------
    ! float32 set_path
    !--------------------------------------------------------------------------

    subroutine c_conduit_node_set_path_float32(cnode, path, val) &
                   bind(C, name="conduit_node_set_path_float32")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(4), value, intent(IN) :: val
    end subroutine c_conduit_node_set_path_float32


    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_float32_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_float32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_float32_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_float32_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_external_float32_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_external_float32_ptr
    

    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_float32_ptr_detailed(cnode, path, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_path_external_float32_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine c_conduit_node_set_path_external_float32_ptr_detailed
    !--------------------------------------------------------------------------
    ! float32 as
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    pure function conduit_node_as_float32(cnode) result(res) &
             bind(C, name="conduit_node_as_float32")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         real(4) :: res
    end function conduit_node_as_float32

    !--------------------------------------------------------------------------
    function c_conduit_node_as_float32_ptr(cnode) result(float32_ptr) &
             bind(C, name="conduit_node_as_float32_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR) :: float32_ptr
     end function c_conduit_node_as_float32_ptr

    !--------------------------------------------------------------------------
    pure function c_conduit_node_fetch_path_as_float32(cnode, path) result(res) &
                   bind(C, name="conduit_node_fetch_path_as_float32")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(4) :: res
    end function c_conduit_node_fetch_path_as_float32

    !--------------------------------------------------------------------------
    function c_conduit_node_fetch_path_as_float32_ptr(cnode, path) result(float32_ptr) &
             bind(C, name="conduit_node_fetch_path_as_float32_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR) :: float32_ptr
     end function c_conduit_node_fetch_path_as_float32_ptr
     


    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! END float32 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! BEGIN float64 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! float64 set
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_float64(cnode, val) &
                   bind(C, name="conduit_node_set_float64")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(8), value, intent(IN) :: val
    end subroutine conduit_node_set_float64

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_float64_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_float64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_float64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_float64_ptr(cnode, data, num_elements) &
                   bind(C, name="conduit_node_set_external_float64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine conduit_node_set_external_float64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_external_float64_ptr_detailed(cnode, data, num_elements,&
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_external_float64_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine conduit_node_set_external_float64_ptr_detailed

    !--------------------------------------------------------------------------
    ! float64 set_path
    !--------------------------------------------------------------------------

    subroutine c_conduit_node_set_path_float64(cnode, path, val) &
                   bind(C, name="conduit_node_set_path_float64")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(8), value, intent(IN) :: val
    end subroutine c_conduit_node_set_path_float64


    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_float64_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_float64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_float64_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_float64_ptr(cnode, path, data, num_elements) &
                   bind(C, name="conduit_node_set_path_external_float64_ptr")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
    end subroutine c_conduit_node_set_path_external_float64_ptr


    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_external_float64_ptr_detailed(cnode, path, data, num_elements, &
                   offset, stride, element_bytes,endianness) &
                   bind(C, name="conduit_node_set_path_external_float64_ptr_detailed")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
    end subroutine c_conduit_node_set_path_external_float64_ptr_detailed


    !--------------------------------------------------------------------------
    ! float64 as
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    pure function conduit_node_as_float64(cnode) result(res) &
             bind(C, name="conduit_node_as_float64")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         real(8) :: res
    end function conduit_node_as_float64

    !--------------------------------------------------------------------------
    function c_conduit_node_as_float64_ptr(cnode) result(float64_ptr) &
             bind(C, name="conduit_node_as_float64_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR) :: float64_ptr
     end function c_conduit_node_as_float64_ptr

    !--------------------------------------------------------------------------
    pure function c_conduit_node_fetch_path_as_float64(cnode, path) result(res) &
                   bind(C, name="conduit_node_fetch_path_as_float64")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        real(8) :: res
    end function c_conduit_node_fetch_path_as_float64

    !--------------------------------------------------------------------------
    function c_conduit_node_fetch_path_as_float64_ptr(cnode, path) result(float64_ptr) &
             bind(C, name="conduit_node_fetch_path_as_float64_ptr")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR) :: float64_ptr
     end function c_conduit_node_fetch_path_as_float64_ptr
     


    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! END float64 methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! BEGIN char8_str methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    ! char8_str set
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_char8_str(cnode,val) &
                   bind(C, name="conduit_node_set_char8_str")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: val(*)
    end subroutine c_conduit_node_set_char8_str

    !--------------------------------------------------------------------------
    ! char8_str set_path
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine c_conduit_node_set_path_char8_str(cnode, path, val) &
                   bind(C, name="conduit_node_set_path_char8_str")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(kind=C_CHAR), intent(IN) :: path(*)
        character(kind=C_CHAR), intent(IN) :: val(*)
    end subroutine c_conduit_node_set_path_char8_str

    !--------------------------------------------------------------------------
    ! char8_str as
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    function c_conduit_node_as_char8_str(cnode) result(str_ptr) &
             bind(C, name="conduit_node_as_char8_str")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR) :: str_ptr
     end function c_conduit_node_as_char8_str


    !--------------------------------------------------------------------------
    function c_conduit_node_fetch_path_as_char8_str(cnode, path) result(str_ptr) &
             bind(C, name="conduit_node_fetch_path_as_char8_str")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         character(kind=C_CHAR), intent(IN) :: path(*)
         type(C_PTR) :: str_ptr
     end function c_conduit_node_fetch_path_as_char8_str

    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! END char8_str methods
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    
    
    subroutine conduit_node_set_int(cnode, val) &
                   bind(C, name="conduit_node_set_int")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(C_INT), value, intent(IN) :: val
    end subroutine conduit_node_set_int

    !--------------------------------------------------------------------------
    pure function conduit_node_as_int(cnode) result(res) &
             bind(C, name="conduit_node_as_int")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         integer(C_INT) :: res
    end function conduit_node_as_int

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_double(cnode, val) &
                   bind(C, name="conduit_node_set_double")
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(C_DOUBLE), value, intent(IN) :: val
    end subroutine conduit_node_set_double
    
    !--------------------------------------------------------------------------
    pure function conduit_node_as_double(cnode) result(res) &
             bind(C, name="conduit_node_as_double")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         real(C_DOUBLE) :: res
    end function conduit_node_as_double

    !--------------------------------------------------------------------------
    pure function conduit_node_dtype(cnode) result(res) &
             bind(C, name="conduit_node_dtype")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         type(C_PTR) :: res
    end function conduit_node_dtype
    
    !--------------------------------------------------------------------------
    ! DataType methods
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    pure function conduit_datatype_sizeof_index_t() result(res) &
             bind(C, name="conduit_datatype_sizeof_index_t")
         use iso_c_binding
         implicit none
         integer(C_INT) :: res
    end function conduit_datatype_sizeof_index_t

    !--------------------------------------------------------------------------
    pure function conduit_datatype_id(cdatatype) result(res) &
             bind(C, name="conduit_datatype_id")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_id
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_name(cdatatype) result(res) &
             bind(C, name="conduit_datatype_name")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         character(kind=C_CHAR) ::res 
    end function c_conduit_datatype_name
    
    !--------------------------------------------------------------------------
    subroutine c_conduit_datatype_name_destroy(nameStr) &
             bind(C, name="conduit_datatype_name_destroy")
         use iso_c_binding
         implicit none
         character(kind=C_CHAR), intent(IN) ::nameStr(*)
    end subroutine c_conduit_datatype_name_destroy

    !--------------------------------------------------------------------------
    pure function conduit_datatype_number_of_elements(cdatatype) result(res) &
            bind(C, name="conduit_datatype_number_of_elements")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_number_of_elements

    !--------------------------------------------------------------------------
    pure function conduit_datatype_offset(cdatatype) result(res) &
            bind(C, name="conduit_datatype_offset")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_offset
    
    !--------------------------------------------------------------------------
    pure function conduit_datatype_stride(cdatatype) result(res) &
            bind(C, name="conduit_datatype_stride")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_stride
    
    !--------------------------------------------------------------------------
    pure function conduit_datatype_element_bytes(cdatatype) result(res) &
            bind(C, name="conduit_datatype_element_bytes")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_element_bytes
    
    !--------------------------------------------------------------------------
    pure function conduit_datatype_element_stride(cdatatype) result(res) &
            bind(C, name="conduit_datatype_element_stride")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_element_stride
    
    !--------------------------------------------------------------------------
    pure function conduit_datatype_endianness(cdatatype) result(res) &
            bind(C, name="conduit_datatype_endianness")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_endianness
    
    !--------------------------------------------------------------------------
    pure function conduit_datatype_element_index(cdatatype, idx) result(res) &
            bind(C, name="conduit_datatype_element_index")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)), value, intent(IN) :: idx
         integer(kind(F_CONDUIT_INDEX_ID)) :: res
    end function conduit_datatype_element_index


    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_empty(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_empty")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_empty
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_object(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_object")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_object
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_list(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_list")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_list
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_number(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_number")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_number

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_floating_point(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_floating_point")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_floating_point


    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_integer(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_integer")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_integer

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_signed_integer(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_signed_integer")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_signed_integer
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_unsigned_integer(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_unsigned_integer")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_unsigned_integer

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_int8(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_int8")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_int8
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_int16(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_int16")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_int16

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_int32(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_int32")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_int32
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_int64(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_int64")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_int64
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_uint8(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_uint8")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_uint8
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_uint16(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_uint16")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_uint16

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_uint32(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_uint32")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_uint32
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_uint64(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_uint64")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_uint64
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_float32(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_float32")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_float32
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_float64(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_float64")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_float64
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_char(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_char")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_char

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_short(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_short")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_short


    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_int(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_int")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_int
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_long(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_long")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_long
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_unsigned_char(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_unsigned_char")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_unsigned_char

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_unsigned_short(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_unsigned_short")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_unsigned_short

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_unsigned_int(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_unsigned_int")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_unsigned_int
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_unsigned_long(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_unsigned_long")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_unsigned_long

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_float(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_float")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_float


    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_double(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_double")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_double

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_string(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_string")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_string

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_char8_str(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_char8_str")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_char8_str

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_little_endian(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_little_endian")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_little_endian
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_big_endian(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_big_endian")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_big_endian

    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_endianness_matches_machine(cdatatype) result(res) &
            bind(C, name="conduit_datatype_endianness_matches_machine")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_endianness_matches_machine
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_stride_element_aligned(cdatatype) result(res) &
            bind(C, name="conduit_datatype_is_stride_element_aligned")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(C_INT) :: res
    end function c_conduit_datatype_is_stride_element_aligned
    
    !--------------------------------------------------------------------------
    pure function c_conduit_datatype_is_stride_aligned(cdatatype, nbytes) result(res) &
            bind(C, name="conduit_datatype_is_stride_aligned")
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cdatatype
         integer(kind(F_CONDUIT_INDEX_ID)), value, intent(IN) :: nbytes
         integer(C_INT) :: res
    end function c_conduit_datatype_is_stride_aligned


    !--------------------------------------------------------------------------
    end interface
    !--------------------------------------------------------------------------

!------------------------------------------------------------------------------
!
contains
!
!------------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    function conduit_node_is_root(cnode) result(res)
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         logical(C_BOOL) :: res
         !-- c interface returns an int, we convert to logical here
         integer(C_INT) :: c_res
         !---
         c_res = c_conduit_node_is_root(cnode)
         if (c_res .EQ. 1) then
             res = .true.
         else
             res = .false.
         endif
     end function conduit_node_is_root

    !--------------------------------------------------------------------------
    function conduit_node_is_data_external(cnode) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        logical(C_BOOL) :: res
        !-- c interface returns an int, we convert to logical here
        integer(C_INT) :: c_res
        !---
        c_res = c_conduit_node_is_data_external(cnode)
        if (c_res .EQ. 1) then
            res = .true.
        else
            res = .false.
        endif
    end function conduit_node_is_data_external

    !--------------------------------------------------------------------------
    function conduit_node_is_compact(cnode) result(res)
         use iso_c_binding
         implicit none
         type(C_PTR), value, intent(IN) :: cnode
         logical(C_BOOL) :: res
         !-- c interface returns an int, we convert to logical here
         integer(C_INT) :: c_res
         !---
         c_res = c_conduit_node_is_compact(cnode)
         if (c_res .EQ. 1) then
             res = .true.
         else
             res = .false.
         endif
     end function conduit_node_is_compact

     !--------------------------------------------------------------------------
     function conduit_node_is_contiguous(cnode) result(res)
      use iso_c_binding
      implicit none
      type(C_PTR), value, intent(IN) :: cnode
          logical(C_BOOL) :: res
          !-- c interface returns an int, we convert to logical here
          integer(C_INT) :: c_res
          !---
          c_res = c_conduit_node_is_contiguous(cnode)
          if (c_res .EQ. 1) then
              res = .true.
          else
              res = .false.
          endif
     end function conduit_node_is_contiguous

     !--------------------------------------------------------------------------
     function conduit_node_contiguous_with_node(cnode,cother) result(res)
       use iso_c_binding
       implicit none
       type(C_PTR), value, intent(IN) :: cnode
       type(C_PTR), value, intent(IN) :: cother
       logical(C_BOOL) :: res
       !-- c interface returns an int, we convert to logical here
       integer(C_INT) :: c_res
       !---
       c_res = c_conduit_node_contiguous_with_node(cnode,cother)
       if (c_res .EQ. 1) then
           res = .true.
       else
           res = .false.
       endif
     end function conduit_node_contiguous_with_node

     !--------------------------------------------------------------------------
     function conduit_node_diff(cnode,cother,cinfo,epsilon) result(res)
       use iso_c_binding
       implicit none
       type(C_PTR), value, intent(IN) :: cnode
       type(C_PTR), value, intent(IN) :: cother
       type(C_PTR), value, intent(IN) :: cinfo
       real(8), value, intent(IN) :: epsilon
       logical(C_BOOL) :: res
       !-- c interface returns an int, we convert to logical here
       integer(C_INT) :: c_res
       !---
       c_res = c_conduit_node_diff(cnode,cother,cinfo,epsilon)
       if (c_res .EQ. 1) then
           res = .true.
       else
           res = .false.
       endif
     end function conduit_node_diff

     !--------------------------------------------------------------------------
     function conduit_node_diff_compatible(cnode,cother,cinfo,epsilon) result(res)
       use iso_c_binding
       implicit none
       type(C_PTR), value, intent(IN) :: cnode
       type(C_PTR), value, intent(IN) :: cother
       type(C_PTR), value, intent(IN) :: cinfo
       real(8), value, intent(IN) :: epsilon
       logical(C_BOOL) :: res
       !-- c interface returns an int, we convert to logical here
       integer(C_INT) :: c_res
       !---
       c_res = c_conduit_node_diff_compatible(cnode,cother,cinfo,epsilon)
       if (c_res .EQ. 1) then
           res = .true.
       else
           res = .false.
       endif
     end function conduit_node_diff_compatible

    !--------------------------------------------------------------------------
    function conduit_node_compatible(cnode,cother) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        type(C_PTR), value, intent(IN) :: cother
        logical(C_BOOL) :: res
        !-- c interface returns an int, we convert to logical here
        integer(C_INT) :: c_res
        !---
        c_res = c_conduit_node_compatible(cnode,cother)
        if (c_res .EQ. 1) then
            res = .true.
        else
            res = .false.
        endif
     end function conduit_node_compatible

    !--------------------------------------------------------------------------
    function conduit_node_fetch(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        type(C_PTR) :: res
        !---
        res = c_conduit_node_fetch(cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_fetch

    !--------------------------------------------------------------------------
    function conduit_node_fetch_existing(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        type(C_PTR) :: res
        !---
        res = c_conduit_node_fetch_existing(cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_fetch_existing

    !--------------------------------------------------------------------------
    function conduit_node_has_child(cnode, name) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: name
        logical(C_BOOL) :: res
        !-- c interface returns an int, we convert to logical here
        integer(C_INT) :: c_res
        !---
        c_res = c_conduit_node_has_child(cnode, trim(name) // C_NULL_CHAR)
        if (c_res .EQ. 1) then
            res = .true.
        else
            res = .false.
        endif
    end function conduit_node_has_child

    !--------------------------------------------------------------------------
    function conduit_node_add_child(cnode, name) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: name
        type(C_PTR) :: res
        !---
        res = c_conduit_node_add_child(cnode, trim(name) // C_NULL_CHAR)
    end function conduit_node_add_child

    !--------------------------------------------------------------------------
    function conduit_node_child_by_name(cnode, name) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: name
        type(C_PTR) :: res
        !---
        res = c_conduit_node_child_by_name(cnode, trim(name) // C_NULL_CHAR)
    end function conduit_node_child_by_name

    !--------------------------------------------------------------------------
    subroutine conduit_node_remove_path(cnode, path)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        !---
        call c_conduit_node_remove_path(cnode, trim(path) // C_NULL_CHAR)
    end subroutine conduit_node_remove_path

    !--------------------------------------------------------------------------
    subroutine conduit_node_remove_child_by_name(cnode, name)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: name
        !---
        call c_conduit_node_remove_child_by_name(cnode, trim(name) // C_NULL_CHAR)
    end subroutine conduit_node_remove_child_by_name

    !--------------------------------------------------------------------------
    subroutine conduit_node_rename_child(cnode, old_name, new_name)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: old_name
        character(*), intent(IN) :: new_name
        !---
        call c_conduit_node_rename_child(cnode, trim(old_name) // C_NULL_CHAR, trim(new_name) // C_NULL_CHAR)
    end subroutine conduit_node_rename_child

    !--------------------------------------------------------------------------
    subroutine conduit_node_parse(cnode, schema, protocol )
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: schema
        character(*), intent(IN) :: protocol
        !---
        call c_conduit_node_parse(cnode, trim(schema) // C_NULL_CHAR, trim(protocol) // C_NULL_CHAR)
    end subroutine conduit_node_parse

    !--------------------------------------------------------------------------
    subroutine conduit_node_save(cnode, path, protocol )
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        character(*), intent(IN) :: protocol
        !---
        call c_conduit_node_save(cnode, trim(path) // C_NULL_CHAR, trim(protocol) // C_NULL_CHAR)
    end subroutine conduit_node_save

    !--------------------------------------------------------------------------
    subroutine conduit_node_load(cnode, path, protocol )
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        character(*), intent(IN) :: protocol
        !---
        call c_conduit_node_load(cnode, trim(path) // C_NULL_CHAR, trim(protocol) // C_NULL_CHAR)
    end subroutine conduit_node_load

    !--------------------------------------------------------------------------
    function conduit_node_has_path(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        logical(C_BOOL) :: res
        !-- c interface returns an int, we convert to logical here
        integer(C_INT) :: c_res
        !---
        c_res = c_conduit_node_has_path(cnode, trim(path) // C_NULL_CHAR)
        if (c_res .EQ. 1) then
            res = .true.
        else
            res = .false.
        endif
    end function conduit_node_has_path
    
    !--------------------------------------------------------------------------
    ! set node subs
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_node(cnode, path, cother)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        type(C_PTR), value, intent(IN) :: cother
        !---
        call c_conduit_node_set_path_node(cnode, trim(path) // C_NULL_CHAR, cother)
    end subroutine conduit_node_set_path_node
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_node(cnode, path, cother)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        type(C_PTR), value, intent(IN) :: cother
        !---
        call c_conduit_node_set_path_external_node(cnode, trim(path) // C_NULL_CHAR, cother)
    end subroutine conduit_node_set_path_external_node

    !--------------------------------------------------------------------------
    ! int32 subs
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_int32(cnode, path, val)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(4), value, intent(IN) :: val
        !---
        call c_conduit_node_set_path_int32(cnode, trim(path) // C_NULL_CHAR, val)
    end subroutine conduit_node_set_path_int32

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_int32_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_int32_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_int32_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_external_int32_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_external_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_int32_ptr_detailed(cnode, path, data, num_elements, &
                   offset, stride, element_bytes,endianness)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
        !---
        call c_conduit_node_set_path_external_int32_ptr_detailed(cnode, trim(path) // C_NULL_CHAR, data, &
        num_elements, offset, stride, element_bytes, endianness)
    end subroutine conduit_node_set_path_external_int32_ptr_detailed

    !--------------------------------------------------------------------------
    function conduit_node_fetch_path_as_int32(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(4) :: res
        !---
        res =  c_conduit_node_fetch_path_as_int32(cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_fetch_path_as_int32

    !--------------------------------------------------------------------------
    subroutine conduit_node_as_int32_ptr(cnode,f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(4), pointer :: f_out(:)
        integer(C_SIZE_T) :: n
        type(C_PTR) :: int32_c_ptr
        !---
        n = conduit_node_number_of_elements(cnode)
        int32_c_ptr = c_conduit_node_as_int32_ptr(cnode)
        call c_f_pointer(int32_c_ptr, f_out, (/n/))
    end subroutine conduit_node_as_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_fetch_path_as_int32_ptr(cnode, path, f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(4), pointer :: f_out(:)
        type(C_PTR) :: sub_node
        !---
        sub_node = c_conduit_node_fetch(cnode,trim(path) // C_NULL_CHAR)
        call conduit_node_as_int32_ptr(sub_node,f_out)
    end subroutine conduit_node_fetch_path_as_int32_ptr

    !--------------------------------------------------------------------------
    ! int64 subs
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_int64(cnode, path, val)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(8), value, intent(IN) :: val
        !---
        call c_conduit_node_set_path_int64(cnode, trim(path) // C_NULL_CHAR, val)
    end subroutine conduit_node_set_path_int64

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_int64_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_int64_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_int64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_int64_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_external_int64_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_external_int64_ptr


    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_int64_ptr_detailed(cnode, path, data, num_elements,&
                   offset, stride, element_bytes,endianness)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
        !---
        call c_conduit_node_set_path_external_int64_ptr_detailed(cnode, trim(path) // C_NULL_CHAR, data,&
        num_elements, offset, stride, element_bytes,endianness)
    end subroutine conduit_node_set_path_external_int64_ptr_detailed


    !--------------------------------------------------------------------------
    function conduit_node_fetch_path_as_int64(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(8) :: res
        !---
        res =  c_conduit_node_fetch_path_as_int64(cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_fetch_path_as_int64

    !--------------------------------------------------------------------------
    subroutine conduit_node_as_int64_ptr(cnode,f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        integer(8), pointer :: f_out(:)
        integer(C_SIZE_T) :: n
        type(C_PTR) :: int64_c_ptr
        !---
        n = conduit_node_number_of_elements(cnode)
        int64_c_ptr = c_conduit_node_as_int64_ptr(cnode)
        call c_f_pointer(int64_c_ptr, f_out, (/n/))
    end subroutine conduit_node_as_int64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_fetch_path_as_int64_ptr(cnode, path, f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        integer(8), pointer :: f_out(:)
        type(C_PTR) :: sub_node
        !---
        sub_node = c_conduit_node_fetch(cnode,trim(path) // C_NULL_CHAR)
        call conduit_node_as_int64_ptr(sub_node,f_out)
    end subroutine conduit_node_fetch_path_as_int64_ptr


    !--------------------------------------------------------------------------
    ! float 32 subs
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_float32(cnode, path, val)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(4), value, intent(IN) :: val
        !---
        call c_conduit_node_set_path_float32(cnode, trim(path) // C_NULL_CHAR, val)
    end subroutine conduit_node_set_path_float32

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_float32_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_float32_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_float32_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_external_float32_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_external_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_float32_ptr_detailed(cnode, path, data, num_elements,&
                   offset, stride, element_bytes,endianness)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
        !---
        call c_conduit_node_set_path_external_float32_ptr_detailed(cnode, trim(path) // C_NULL_CHAR, data, &
        num_elements, offset, stride, element_bytes,endianness)
    end subroutine conduit_node_set_path_external_float32_ptr_detailed


    !--------------------------------------------------------------------------
    function conduit_node_fetch_path_as_float32(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(4) :: res
        !---
        res =  c_conduit_node_fetch_path_as_float32(cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_fetch_path_as_float32

    !--------------------------------------------------------------------------
    subroutine conduit_node_as_float32_ptr(cnode,f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(4), pointer :: f_out(:)
        integer(C_SIZE_T) :: n
        type(C_PTR) :: float32_c_ptr
        !---
        n = conduit_node_number_of_elements(cnode)
        float32_c_ptr = c_conduit_node_as_float32_ptr(cnode)
        call c_f_pointer(float32_c_ptr, f_out, (/n/))
    end subroutine conduit_node_as_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_fetch_path_as_float32_ptr(cnode, path, f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(4), pointer :: f_out(:)
        type(C_PTR) :: sub_node
        !---
        sub_node = c_conduit_node_fetch(cnode,trim(path) // C_NULL_CHAR)
        call conduit_node_as_float32_ptr(sub_node,f_out)
    end subroutine conduit_node_fetch_path_as_float32_ptr



    !--------------------------------------------------------------------------
    ! float 64 subs
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_float64(cnode, path, val)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(8), value, intent(IN) :: val
        !---
        call c_conduit_node_set_path_float64(cnode, trim(path) // C_NULL_CHAR, val)
    end subroutine conduit_node_set_path_float64

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_float64_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_float64_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_float64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_float64_ptr(cnode, path, data, num_elements)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        !---
        call c_conduit_node_set_path_external_float64_ptr(cnode, trim(path) // C_NULL_CHAR, data, num_elements)
    end subroutine conduit_node_set_path_external_float64_ptr


    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_external_float64_ptr_detailed(cnode, path, data, num_elements,&
                   offset, stride, element_bytes,endianness)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T), value, intent(in) :: num_elements
        integer(C_SIZE_T), value, intent(in) :: offset
        integer(C_SIZE_T), value, intent(in) :: stride
        integer(C_SIZE_T), value, intent(in) :: element_bytes
        integer(C_SIZE_T), value, intent(in) :: endianness
        !---
        call c_conduit_node_set_path_external_float64_ptr_detailed(cnode, trim(path) // C_NULL_CHAR, data, &
        num_elements, offset, stride, element_bytes,endianness)
    end subroutine conduit_node_set_path_external_float64_ptr_detailed


    !--------------------------------------------------------------------------
    function conduit_node_fetch_path_as_float64(cnode, path) result(res)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(8) :: res
        !---
        res =  c_conduit_node_fetch_path_as_float64(cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_fetch_path_as_float64

    !--------------------------------------------------------------------------
    subroutine conduit_node_as_float64_ptr(cnode,f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        real(8), pointer :: f_out(:)
        integer(C_SIZE_T) :: n
        type(C_PTR) :: float64_c_ptr
        !---
        n = conduit_node_number_of_elements(cnode)
        float64_c_ptr = c_conduit_node_as_float64_ptr(cnode)
        call c_f_pointer(float64_c_ptr, f_out, (/n/))
    end subroutine conduit_node_as_float64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_fetch_path_as_float64_ptr(cnode, path, f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        real(8), pointer :: f_out(:)
        type(C_PTR) :: sub_node
        !---
        sub_node = c_conduit_node_fetch(cnode,trim(path) // C_NULL_CHAR)
        call conduit_node_as_float64_ptr(sub_node,f_out)
    end subroutine conduit_node_fetch_path_as_float64_ptr


    !--------------------------------------------------------------------------
    ! char8_str subs
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_char8_str(cnode,val)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: val
        call c_conduit_node_set_char8_str(cnode,trim(val) // C_NULL_CHAR)
    end subroutine conduit_node_set_char8_str
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_set_path_char8_str(cnode, path, val)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        character(*), intent(IN) :: val
        call c_conduit_node_set_path_char8_str(cnode, trim(path) // C_NULL_CHAR, trim(val) // C_NULL_CHAR)
    end subroutine conduit_node_set_path_char8_str

    !--------------------------------------------------------------------------
    subroutine conduit_node_as_char8_str(cnode, f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character,pointer :: f_out(:)
        integer(C_SIZE_T) :: n
        type(C_PTR) :: str_c_ptr
        !---
        n = conduit_node_number_of_elements(cnode)
        str_c_ptr = c_conduit_node_as_char8_str(cnode)
        call c_f_pointer(str_c_ptr, f_out, (/n/))
    end subroutine conduit_node_as_char8_str
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_fetch_path_as_char8_str(cnode, path, f_out)
        use iso_c_binding
        implicit none
        type(C_PTR), value, intent(IN) :: cnode
        character(*), intent(IN) :: path
        character,pointer :: f_out(:)
        type(C_PTR) :: sub_node
        !---
        sub_node = c_conduit_node_fetch(cnode,trim(path) // C_NULL_CHAR)
        call conduit_node_as_char8_str(sub_node,f_out)
    end subroutine conduit_node_fetch_path_as_char8_str


!------------------------------------------------------------------------------
end module conduit
!------------------------------------------------------------------------------

