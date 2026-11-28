!* Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
!* Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
!* other details. No copyright assignment is required to contribute to Conduit.

!------------------------------------------------------------------------------
! conduit_fortran_obj.f
!------------------------------------------------------------------------------

!------------------------------------------------------------------------------
module conduit_obj
!------------------------------------------------------------------------------
    use conduit
    use, intrinsic :: iso_c_binding, only : C_PTR
    implicit none
    
    !--------------------------------------------------------------------------
    type node
        type(C_PTR) cnode
    contains

        !----------------------------------------------------------------------
        procedure :: fetch  => conduit_node_obj_fetch
        procedure :: fetch_existing  => conduit_node_obj_fetch_existing
        procedure :: append => conduit_node_obj_append
        procedure :: child_by_index => conduit_node_obj_child
        procedure :: child_by_name  => conduit_node_obj_child_by_name
        procedure :: parent => conduit_node_obj_parent
        procedure :: info   => conduit_node_obj_info
        !----------------------------------------------------------------------
        procedure :: number_of_children => conduit_node_obj_number_of_children
        procedure :: number_of_elements => conduit_node_obj_number_of_elements
        !---------------------------------------------------------------------
        procedure :: total_strided_bytes => conduit_node_obj_total_strided_bytes
        procedure :: total_bytes_compact => conduit_node_obj_total_bytes_compact
        procedure :: total_bytes_allocated => conduit_node_obj_total_bytes_allocated
        !----------------------------------------------------------------------
        procedure :: print => conduit_node_obj_print
        procedure :: print_detailed => conduit_node_obj_print_detailed
        !----------------------------------------------------------------------
        procedure :: compact_to => conduit_node_obj_compact_to
        !----------------------------------------------------------------------

        !----------------------------------------------------------------------
        procedure :: is_root => conduit_node_obj_is_root
        procedure :: is_contiguous => conduit_node_obj_is_contiguous
        procedure :: is_data_external => conduit_node_obj_is_data_external

        !----------------------------------------------------------------------
        procedure :: diff => conduit_node_obj_diff
        procedure :: diff_compatible => conduit_node_obj_diff_compatible

        !----------------------------------------------------------------------
        procedure :: update => conduit_node_obj_update
        procedure :: update_compatible => conduit_node_obj_update_compatible
        procedure :: update_external => conduit_node_obj_update_external

        !----------------------------------------------------------------------
        procedure :: parse => conduit_node_obj_parse
        procedure :: save  => conduit_node_obj_save
        procedure :: load  => conduit_node_obj_load

        !----------------------------------------------------------------------
        procedure :: add_child  => conduit_node_obj_add_child
        procedure :: has_child  => conduit_node_obj_has_child
        procedure :: has_path   => conduit_node_obj_has_path
        procedure :: rename_child => conduit_node_obj_rename_child

        !----------------------------------------------------------------------
        procedure :: remove_path  => conduit_node_obj_remove_path
        procedure :: remove_child_by_index => conduit_node_obj_remove_child
        procedure :: remove_child_by_name  => conduit_node_obj_remove_child_by_name

        !----------------------------------------------------------------------
        procedure :: reset => conduit_node_obj_reset

        !----------------------------------------------------------------------
        procedure :: move => conduit_node_obj_move
        procedure :: swap => conduit_node_obj_swap

        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        ! begin node cases
        !----------------------------------------------------------------------
        procedure :: set_node    => conduit_node_obj_set_node
        procedure :: set_external_node => conduit_node_obj_set_external_node
        !----------------------------------------------------------------------
        procedure :: set_path_node => conduit_node_obj_set_path_node
        procedure :: set_path_external_node => conduit_node_obj_set_path_external_node
        !----------------------------------------------------------------------
        ! end node cases
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------

        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        ! begin int32 cases
        !----------------------------------------------------------------------
        procedure :: set_int32     => conduit_node_obj_set_int32
        procedure :: set_int32_ptr => conduit_node_obj_set_int32_ptr
        procedure :: set_external_int32_ptr => conduit_node_obj_set_external_int32_ptr
        !----------------------------------------------------------------------
        procedure :: set_path_int32 => conduit_node_obj_set_path_int32
        procedure :: set_path_int32_ptr => conduit_node_obj_set_path_int32_ptr
        procedure :: set_path_external_int32_ptr => conduit_node_obj_set_path_external_int32_ptr

        procedure :: as_int32      => conduit_node_obj_as_int
        procedure :: as_int32_ptr  => conduit_node_obj_as_int32_ptr
        !----------------------------------------------------------------------
        procedure :: fetch_path_as_int32 => conduit_node_obj_fetch_path_as_int32
        procedure :: fetch_path_as_int32_ptr => conduit_node_obj_fetch_path_as_int32_ptr
        !----------------------------------------------------------------------
        ! end int32 cases
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------

        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        ! begin int64 cases
        !----------------------------------------------------------------------
        procedure :: set_int64     => conduit_node_obj_set_int64
        procedure :: set_int64_ptr => conduit_node_obj_set_int64_ptr
        procedure :: set_external_int64_ptr => conduit_node_obj_set_external_int64_ptr
        !----------------------------------------------------------------------
        procedure :: set_path_int64 => conduit_node_obj_set_path_int64
        procedure :: set_path_int64_ptr => conduit_node_obj_set_path_int64_ptr
        procedure :: set_path_external_int64_ptr => conduit_node_obj_set_path_external_int64_ptr

        procedure :: as_int64      => conduit_node_obj_as_int
        procedure :: as_int64_ptr  => conduit_node_obj_as_int64_ptr
        !----------------------------------------------------------------------
        procedure :: fetch_path_as_int64 => conduit_node_obj_fetch_path_as_int64
        procedure :: fetch_path_as_int64_ptr => conduit_node_obj_fetch_path_as_int64_ptr
        !----------------------------------------------------------------------
        ! end int64 cases
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------

        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        ! begin float32 cases
        !----------------------------------------------------------------------
        procedure :: set_float32     => conduit_node_obj_set_float32 
        procedure :: set_float32_ptr => conduit_node_obj_set_float32_ptr
        procedure :: set_external_float32_ptr => conduit_node_obj_set_external_float32_ptr
        !----------------------------------------------------------------------
        procedure :: set_path_float32  => conduit_node_obj_set_path_float32 
        procedure :: set_path_float32_ptr => conduit_node_obj_set_path_float32_ptr
        procedure :: set_path_external_float32_ptr => conduit_node_obj_set_path_external_float32_ptr

        procedure :: as_float32      => conduit_node_obj_as_float32
        procedure :: as_float32_ptr  => conduit_node_obj_as_float32_ptr
        !----------------------------------------------------------------------
        procedure :: fetch_path_as_float32  => conduit_node_obj_fetch_path_as_float32 
        procedure :: fetch_path_as_float32_ptr => conduit_node_obj_fetch_path_as_float32_ptr 
        
        !----------------------------------------------------------------------
        ! end float32 cases
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------

        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        ! begin float64 cases
        !----------------------------------------------------------------------
        procedure :: set_float64     => conduit_node_obj_set_float64 
        procedure :: set_float64_ptr => conduit_node_obj_set_float64_ptr
        procedure :: set_external_float64_ptr => conduit_node_obj_set_external_float64_ptr
        !----------------------------------------------------------------------
        procedure :: set_path_float64  => conduit_node_obj_set_path_float64 
        procedure :: set_path_float64_ptr => conduit_node_obj_set_path_float64_ptr
        procedure :: set_path_external_float64_ptr => conduit_node_obj_set_path_external_float64_ptr

        procedure :: as_float64      => conduit_node_obj_as_float64
        procedure :: as_float64_ptr  => conduit_node_obj_as_float64_ptr
        !----------------------------------------------------------------------
        procedure :: fetch_path_as_float64  => conduit_node_obj_fetch_path_as_float64 
        procedure :: fetch_path_as_float64_ptr => conduit_node_obj_fetch_path_as_float64_ptr 
        
        !----------------------------------------------------------------------
        ! end float64 cases
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        ! begin char8 cases
        !----------------------------------------------------------------------
        procedure :: set_char8_str => conduit_node_obj_set_char8_str
        procedure :: set_path_char8_str => conduit_node_obj_set_path_char8_str
        !----------------------------------------------------------------------
        procedure :: as_char8_str => conduit_node_obj_as_char8_str
        procedure :: fetch_path_as_char8_str => conduit_node_obj_fetch_path_as_char8_str
        
        !----------------------------------------------------------------------
        ! end char8 cases
        !----------------------------------------------------------------------
        !----------------------------------------------------------------------
        
        !----------------------------------------------------------------------
        ! generic methods
        !----------------------------------------------------------------------

        !----------------------------------------------------------------------

        generic :: remove  => remove_path

        generic :: remove_child  => remove_child_by_index, &
                                    remove_child_by_name


        generic :: set  => set_node, &
                           set_int32, &
                           set_int64, &
                           set_float32, &
                           set_float64, &
                           set_char8_str

        generic :: child  => child_by_index, &
                             child_by_name

        generic :: set_ptr  => set_int32_ptr, &
                               set_int64_ptr, &
                               set_float32_ptr, &
                               set_float64_ptr

        !----------------------------------------------------------------------
        generic :: set_path  => set_path_node, &
                                set_path_int32,  &
                                set_path_int64,  &
                                set_path_float32, &
                                set_path_float64, &
                                set_path_char8_str

        generic :: set_path_ptr  => set_path_int32_ptr, &
                                    set_path_int64_ptr, &
                                    set_path_float32_ptr, &
                                    set_path_float64_ptr

        !----------------------------------------------------------------------
        generic :: set_external  => set_external_node, &
                                    set_external_int32_ptr, &
                                    set_external_int64_ptr, &
                                    set_external_float32_ptr, &
                                    set_external_float64_ptr

        !----------------------------------------------------------------------
        generic :: set_path_external  => set_path_external_node, &
                                         set_path_external_int32_ptr, &
                                         set_path_external_int64_ptr, &
                                         set_path_external_float32_ptr, &
                                         set_path_external_float64_ptr


        !----------------------------------------------------------------------
        procedure :: set_int    => conduit_node_obj_set_int
        procedure :: as_int     => conduit_node_obj_as_int
        !----------------------------------------------------------------------
        procedure :: set_double => conduit_node_obj_set_double
        procedure :: as_double  => conduit_node_obj_as_double


    end type node
    !--------------------------------------------------------------------------

!------------------------------------------------------------------------------
!
contains
!
!------------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    function conduit_node_obj_create() result(obj)
        use iso_c_binding
        implicit none
        type(node) :: obj
        obj%cnode = conduit_node_create()
    end function conduit_node_obj_create

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_destroy(obj)
        use iso_c_binding
        implicit none
        class(node) :: obj
        call conduit_node_destroy(obj%cnode)
        obj%cnode = C_NULL_PTR
    end subroutine conduit_node_obj_destroy

    !--------------------------------------------------------------------------
    function conduit_node_obj_is_root(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        logical(C_BOOL) :: res
        res = conduit_node_is_root(obj%cnode)
     end function conduit_node_obj_is_root

    !--------------------------------------------------------------------------
    function conduit_node_obj_is_contiguous(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        logical(C_BOOL) :: res
        res = conduit_node_is_contiguous(obj%cnode)
    end function conduit_node_obj_is_contiguous


    !--------------------------------------------------------------------------
    function conduit_node_obj_is_data_external(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        logical(C_BOOL) :: res
        res = conduit_node_is_data_external(obj%cnode)
     end function conduit_node_obj_is_data_external

    !--------------------------------------------------------------------------
    function conduit_node_obj_is_compact(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        logical(C_BOOL) :: res
        res = conduit_node_is_compact(obj%cnode)
    end function conduit_node_obj_is_compact

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_compact_to(obj,other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        call conduit_node_compact_to(obj%cnode, other%cnode)
    end subroutine conduit_node_obj_compact_to

    !--------------------------------------------------------------------------
    function conduit_node_obj_diff(obj,other,info,epsilon) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        class(node) :: info
        real(8) :: epsilon
        logical(C_BOOL) :: res
        res = conduit_node_diff(obj%cnode, other%cnode, info%cnode, epsilon)
    end function conduit_node_obj_diff

    !--------------------------------------------------------------------------
    function conduit_node_obj_diff_compatible(obj,other,info,epsilon) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        class(node) :: info
        real(8) :: epsilon
        logical(C_BOOL) :: res
        res = conduit_node_diff_compatible(obj%cnode, other%cnode, info%cnode, epsilon)
    end function conduit_node_obj_diff_compatible

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_reset(obj)
        use iso_c_binding
        implicit none
        class(node) :: obj
        call conduit_node_reset(obj%cnode)
    end subroutine conduit_node_obj_reset

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_move(obj_a,obj_b)
        use iso_c_binding
        implicit none
        class(node) :: obj_a
        class(node) :: obj_b
        call conduit_node_move(obj_a%cnode,obj_b%cnode)
    end subroutine conduit_node_obj_move

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_swap(obj_a,obj_b)
        use iso_c_binding
        implicit none
        class(node) :: obj_a
        class(node) :: obj_b
        call conduit_node_swap(obj_a%cnode,obj_b%cnode)
    end subroutine conduit_node_obj_swap

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_update(obj,other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        call conduit_node_update(obj%cnode, other%cnode)
    end subroutine conduit_node_obj_update

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_update_compatible(obj,other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        call conduit_node_update_compatible(obj%cnode, other%cnode)
    end subroutine conduit_node_obj_update_compatible

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_update_external(obj,other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        call conduit_node_update_external(obj%cnode, other%cnode)
    end subroutine conduit_node_obj_update_external

    !--------------------------------------------------------------------------
    ! basic io, parsing, generation
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_parse(obj,schema,protocol)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: schema
        character(*) :: protocol
        call conduit_node_parse(obj%cnode, schema, protocol)
    end subroutine conduit_node_obj_parse

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_save(obj,path,protocol)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        character(*) :: protocol
        call conduit_node_save(obj%cnode, path, protocol)
    end subroutine conduit_node_obj_save

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_load(obj,path,protocol)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        character(*) :: protocol
        call conduit_node_load(obj%cnode, path, protocol)
    end subroutine conduit_node_obj_load


    !--------------------------------------------------------------------------
    function conduit_node_obj_fetch(obj, path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        type(node) :: res
        res%cnode = conduit_node_fetch(obj%cnode, trim(path) // C_NULL_CHAR)
     end function conduit_node_obj_fetch

    !--------------------------------------------------------------------------
    function conduit_node_obj_fetch_existing(obj, path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        type(node) :: res
        res%cnode = conduit_node_fetch_existing(obj%cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_obj_fetch_existing

    !--------------------------------------------------------------------------
    function conduit_node_obj_append(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        type(node) :: res
        res%cnode = conduit_node_append(obj%cnode)
    end function conduit_node_obj_append

    !--------------------------------------------------------------------------
    function conduit_node_obj_add_child(obj, name) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: name
        type(node) :: res
        res%cnode = conduit_node_add_child(obj%cnode, name)
    end function conduit_node_obj_add_child

    !--------------------------------------------------------------------------
    function conduit_node_obj_child(obj, idx) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: idx
        type(node) :: res
        res%cnode = conduit_node_child(obj%cnode, idx)
    end function conduit_node_obj_child

    !--------------------------------------------------------------------------
    function conduit_node_obj_child_by_name(obj, name) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: name
        type(node) :: res
        res%cnode = conduit_node_child_by_name(obj%cnode, name)
    end function conduit_node_obj_child_by_name

    !--------------------------------------------------------------------------
    function conduit_node_obj_has_child(obj, name) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: name
        logical(C_BOOL) :: res
        res = conduit_node_has_child(obj%cnode, name)
    end function conduit_node_obj_has_child

    !--------------------------------------------------------------------------
    function conduit_node_obj_has_path(obj, path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        logical(C_BOOL) :: res
        res = conduit_node_has_path(obj%cnode, path)
    end function conduit_node_obj_has_path

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_remove_path(obj, path)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        call conduit_node_remove_path(obj%cnode, path)
    end subroutine conduit_node_obj_remove_path

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_remove_child(obj, idx)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: idx
        call conduit_node_remove_child(obj%cnode, idx)
    end subroutine conduit_node_obj_remove_child

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_remove_child_by_name(obj, name)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: name
        call conduit_node_remove_child_by_name(obj%cnode, name)
    end subroutine conduit_node_obj_remove_child_by_name

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_rename_child(obj, old_name, new_name)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: old_name
        character(*) :: new_name
        call conduit_node_rename_child(obj%cnode, old_name, new_name)
    end subroutine conduit_node_obj_rename_child

    !--------------------------------------------------------------------------
    function conduit_node_obj_parent(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        type(node) :: res
        res%cnode = conduit_node_parent(obj%cnode)
    end function conduit_node_obj_parent

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_info(obj,oinfo)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: oinfo
        call conduit_node_info(obj%cnode, oinfo%cnode)
    end subroutine conduit_node_obj_info

    !--------------------------------------------------------------------------
    function conduit_node_obj_compatible(obj, other) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        logical(C_BOOL) :: res
        res = conduit_node_compatible(obj%cnode,other%cnode)
    end function conduit_node_obj_compatible

    !--------------------------------------------------------------------------
    function conduit_node_obj_number_of_children(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: res
        res = conduit_node_number_of_children(obj%cnode)
    end function conduit_node_obj_number_of_children

    !--------------------------------------------------------------------------
    function conduit_node_obj_number_of_elements(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: res
        res = conduit_node_number_of_elements(obj%cnode)
    end function conduit_node_obj_number_of_elements

    !--------------------------------------------------------------------------
    function conduit_node_obj_total_strided_bytes(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: res
        res = conduit_node_total_strided_bytes(obj%cnode)
    end function conduit_node_obj_total_strided_bytes

    !--------------------------------------------------------------------------
    function conduit_node_obj_total_bytes_compact(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: res
        res = conduit_node_total_bytes_compact(obj%cnode)
    end function conduit_node_obj_total_bytes_compact

    !--------------------------------------------------------------------------
    function conduit_node_obj_total_bytes_allocated(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_SIZE_T) :: res
        res = conduit_node_total_bytes_allocated(obj%cnode)
    end function conduit_node_obj_total_bytes_allocated
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! begin node
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_node(obj, other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        call conduit_node_set_node(obj%cnode, other%cnode)
    end subroutine conduit_node_obj_set_node

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_external_node(obj, other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        class(node) :: other
        call conduit_node_set_external_node(obj%cnode, other%cnode)
    end subroutine conduit_node_obj_set_external_node

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_node(obj, path, other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        class(node) :: other
        call conduit_node_set_path_node(obj%cnode, path, other%cnode)
    end subroutine conduit_node_obj_set_path_node

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_external_node(obj, path, other)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        class(node) :: other
        call conduit_node_set_path_external_node(obj%cnode, path, other%cnode)
    end subroutine conduit_node_obj_set_path_external_node

    !--------------------------------------------------------------------------
    ! end node
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------



    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! begin int32
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_int32(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(4) :: val
        call conduit_node_set_int32(obj%cnode, val)
    end subroutine conduit_node_obj_set_int32

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_int32_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_int32_ptr(obj%cnode,data,num_elements)
    end subroutine conduit_node_obj_set_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_external_int32_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(4), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_external_int32_ptr(obj%cnode, data, num_elements)
    end subroutine conduit_node_obj_set_external_int32_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_int32(obj, path, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(4) :: val
        call conduit_node_set_path_int32(obj%cnode, path, val)
    end subroutine conduit_node_obj_set_path_int32

    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_int32_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_int32_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_int32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_external_int32_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(4), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_external_int32_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_external_int32_ptr

    
    !--------------------------------------------------------------------------
    function conduit_node_obj_as_int32(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(4) :: res
        res = conduit_node_as_int32(obj%cnode)
    end function conduit_node_obj_as_int32
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_as_int32_ptr(obj,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(4), pointer :: f_out(:)
        call conduit_node_as_int32_ptr(obj%cnode,f_out)
    end subroutine conduit_node_obj_as_int32_ptr

    !--------------------------------------------------------------------------
    function conduit_node_obj_fetch_path_as_int32(obj,path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(4) :: res
        res = conduit_node_fetch_path_as_int32(obj%cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_obj_fetch_path_as_int32

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_fetch_path_as_int32_ptr(obj,path,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(4), pointer :: f_out(:)
        call conduit_node_fetch_path_as_int32_ptr(obj%cnode,path,f_out)
    end subroutine conduit_node_obj_fetch_path_as_int32_ptr

    !--------------------------------------------------------------------------
    ! end int32
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    
    
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! begin int64
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_int64(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(8) :: val
        call conduit_node_set_int64(obj%cnode, val)
    end subroutine conduit_node_obj_set_int64

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_int64_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_int64_ptr(obj%cnode,data,num_elements)
    end subroutine conduit_node_obj_set_int64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_external_int64_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(8), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_external_int64_ptr(obj%cnode, data, num_elements)
    end subroutine conduit_node_obj_set_external_int64_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_int64(obj, path, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(8) :: val
        call conduit_node_set_path_int64(obj%cnode, path, val)
    end subroutine conduit_node_obj_set_path_int64

    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_int64_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_int64_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_int64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_external_int64_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(8), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_external_int64_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_external_int64_ptr

    
    !--------------------------------------------------------------------------
    function conduit_node_obj_as_int64(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(8) :: res
        res = conduit_node_as_int64(obj%cnode)
    end function conduit_node_obj_as_int64
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_as_int64_ptr(obj,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(8), pointer :: f_out(:)
        call conduit_node_as_int64_ptr(obj%cnode,f_out)
    end subroutine conduit_node_obj_as_int64_ptr

    !--------------------------------------------------------------------------
    function conduit_node_obj_fetch_path_as_int64(obj,path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(8) :: res
        res = conduit_node_fetch_path_as_int64(obj%cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_obj_fetch_path_as_int64

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_fetch_path_as_int64_ptr(obj,path,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        integer(8), pointer :: f_out(:)
        call conduit_node_fetch_path_as_int64_ptr(obj%cnode,path,f_out)
    end subroutine conduit_node_obj_fetch_path_as_int64_ptr

    !--------------------------------------------------------------------------
    ! end int64
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! begin float32
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_float32(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(4) :: val
        call conduit_node_set_float32(obj%cnode, val)
    end subroutine conduit_node_obj_set_float32

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_float32_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_float32_ptr(obj%cnode,data,num_elements)
    end subroutine conduit_node_obj_set_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_external_float32_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(4), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_external_float32_ptr(obj%cnode, data, num_elements)
    end subroutine conduit_node_obj_set_external_float32_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_float32(obj, path, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(4) :: val
        call conduit_node_set_path_float32(obj%cnode, path, val)
    end subroutine conduit_node_obj_set_path_float32

    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_float32_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(4), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_float32_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_float32_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_external_float32_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(4), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_external_float32_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_external_float32_ptr

    
    !--------------------------------------------------------------------------
    function conduit_node_obj_as_float32(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(4) :: res
        res = conduit_node_as_float32(obj%cnode)
    end function conduit_node_obj_as_float32
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_as_float32_ptr(obj,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(4), pointer :: f_out(:)
        call conduit_node_as_float32_ptr(obj%cnode,f_out)
    end subroutine conduit_node_obj_as_float32_ptr

    !--------------------------------------------------------------------------
    function conduit_node_obj_fetch_path_as_float32(obj,path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(4) :: res
        res = conduit_node_fetch_path_as_float32(obj%cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_obj_fetch_path_as_float32

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_fetch_path_as_float32_ptr(obj,path,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(4), pointer :: f_out(:)
        call conduit_node_fetch_path_as_float32_ptr(obj%cnode,path,f_out)
    end subroutine conduit_node_obj_fetch_path_as_float32_ptr

    !--------------------------------------------------------------------------
    ! end float32
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! begin float64
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_float64(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(8) :: val
        call conduit_node_set_float64(obj%cnode, val)
    end subroutine conduit_node_obj_set_float64

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_float64_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_float64_ptr(obj%cnode,data,num_elements)
    end subroutine conduit_node_obj_set_float64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_external_float64_ptr(obj, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(8), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_external_float64_ptr(obj%cnode, data, num_elements)
    end subroutine conduit_node_obj_set_external_float64_ptr
    
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_float64(obj, path, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(8) :: val
        call conduit_node_set_path_float64(obj%cnode, path, val)
    end subroutine conduit_node_obj_set_path_float64

    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_float64_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(8), intent (IN), dimension (*) :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_float64_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_float64_ptr

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_external_float64_ptr(obj, path, data, num_elements) 
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(8), intent (IN), dimension (*), target :: data
        integer(C_SIZE_T) :: num_elements
        call conduit_node_set_path_external_float64_ptr(obj%cnode, path, data, num_elements)
    end subroutine conduit_node_obj_set_path_external_float64_ptr

    
    !--------------------------------------------------------------------------
    function conduit_node_obj_as_float64(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(8) :: res
        res = conduit_node_as_float64(obj%cnode)
    end function conduit_node_obj_as_float64
    
    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_as_float64_ptr(obj,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(8), pointer :: f_out(:)
        call conduit_node_as_float64_ptr(obj%cnode,f_out)
    end subroutine conduit_node_obj_as_float64_ptr

    !--------------------------------------------------------------------------
    function conduit_node_obj_fetch_path_as_float64(obj,path) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(8) :: res
        res = conduit_node_fetch_path_as_float64(obj%cnode, trim(path) // C_NULL_CHAR)
    end function conduit_node_obj_fetch_path_as_float64

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_fetch_path_as_float64_ptr(obj,path,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        real(8), pointer :: f_out(:)
        call conduit_node_fetch_path_as_float64_ptr(obj%cnode,path,f_out)
    end subroutine conduit_node_obj_fetch_path_as_float64_ptr

    !--------------------------------------------------------------------------
    ! end float64
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    ! begin char8_str
    !--------------------------------------------------------------------------

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_char8_str(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: val
        call conduit_node_set_char8_str(obj%cnode, val)
    end subroutine conduit_node_obj_set_char8_str

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_path_char8_str(obj, path, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        character(*) :: val
        call conduit_node_set_path_char8_str(obj%cnode, path, val)
    end subroutine conduit_node_obj_set_path_char8_str

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_as_char8_str(obj,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character, pointer :: f_out(:)
        call conduit_node_as_char8_str(obj%cnode,f_out)
    end subroutine conduit_node_obj_as_char8_str

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_fetch_path_as_char8_str(obj,path,f_out)
        use iso_c_binding
        implicit none
        class(node) :: obj
        character(*) :: path
        character, pointer :: f_out(:)
        call conduit_node_fetch_path_as_char8_str(obj%cnode,path,f_out)
    end subroutine conduit_node_obj_fetch_path_as_char8_str

    !--------------------------------------------------------------------------
    ! end char8_str
    !--------------------------------------------------------------------------
    !--------------------------------------------------------------------------
    

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_int(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_INT) :: val
        call conduit_node_set_int(obj%cnode, val)
    end subroutine conduit_node_obj_set_int

    !--------------------------------------------------------------------------
    function conduit_node_obj_as_int(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        integer(C_INT) :: res
        res = conduit_node_as_int(obj%cnode)
    end function conduit_node_obj_as_int

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_set_double(obj, val)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(C_DOUBLE) :: val
        call conduit_node_set_double(obj%cnode, val)
    end subroutine conduit_node_obj_set_double

    !--------------------------------------------------------------------------
    function conduit_node_obj_as_double(obj) result(res)
        use iso_c_binding
        implicit none
        class(node) :: obj
        real(C_DOUBLE) :: res
        res = conduit_node_as_double(obj%cnode)
    end function conduit_node_obj_as_double


    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_print(obj)
        use iso_c_binding
        implicit none
        class(node) :: obj
        call conduit_node_print(obj%cnode)
    end subroutine conduit_node_obj_print

    !--------------------------------------------------------------------------
    subroutine conduit_node_obj_print_detailed(obj)
        use iso_c_binding
        implicit none
        class(node) :: obj
        call conduit_node_print_detailed(obj%cnode)
    end subroutine conduit_node_obj_print_detailed

!------------------------------------------------------------------------------
end module conduit_obj
!------------------------------------------------------------------------------


