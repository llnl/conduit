// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_partition.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_PARTITION_HPP
#define CONDUIT_BLUEPRINT_MESH_PARTITION_HPP

//-----------------------------------------------------------------------------
// std includes
//-----------------------------------------------------------------------------
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_exports.h"

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{

/**
 @brief Base class for selections that identify regions of interest that will
        be extracted from a mesh.
 */
class CONDUIT_BLUEPRINT_API Selection
{
public:
    static constexpr int FREE_DOMAIN_ID = -1; // The domain number needs to be assigned.
    static constexpr int FREE_RANK_ID = -1;   // The rank needs to be assigned.

    Selection();
    Selection(const Selection &obj);
    virtual ~Selection();

    /**
     Create a copy of the selection.
     */
    virtual std::shared_ptr<Selection> copy() const = 0;

    /**
     @brief Initializes the selection from the provided Conduit node.
     @param n_options A Conduit node containing the options for this
                      selection.
     @return True if the selection was initialized successfully; False otherwise.
     */
    virtual bool init(const conduit::Node &n_options);

    /**
     @brief Determines whether the selection can be applied to the supplied mesh.
     @note  This is a check that the selection type makes sense for the input
            mesh type.
     @param n_mesh A Conduit conduit::Node containing the mesh.
     @return True if the selection can be applied to the mesh; False otherwise.
     */
    virtual bool applicable(const conduit::Node &n_mesh) = 0;

    /**
     @brief Return the number of cells in the selection.
     @return The number of cells in the selection.
     */
    virtual index_t length(const conduit::Node &n_mesh) const;

    /**
     @brief Returns whether the selection covers the whole mesh. Selections that
            covered a subset of the mesh or were otherwise partitioned would
            not be whole.
     @param n_mesh A Conduit conduit::Node containing the mesh.
     @return True of the mesh is whole. False otherwise.
     */
    virtual bool get_whole(const conduit::Node &n_mesh);

    /**
     @brief Set whether the selection is considered to cover the whole mesh.
     @param value A value indicating whether the selection covers the mesh.
     */
    void set_whole(bool value);

    /**
     @brief Returns whether the selection must be partitioned when we create it.
     */
    virtual bool requires_initial_partition() const;

    /**
     @brief Partitions the selection into smaller selections.
     @param n_mesh A Conduit conduit::Node containing the mesh.
     @return A vector of selection pointers that cover the input selection.
     */
    virtual std::vector<std::shared_ptr<Selection> > partition(const conduit::Node &n_mesh) const = 0;

    /**
     @brief Return the domain index to which the selection is being applied.
     @return The domain index or 0. This value is 0 by default.
     */
    index_t get_domain() const;

    /**
     @brief Set the domain index to which the selection is being applied.
     @param value The new domain index.
     */
    void set_domain(index_t value);

    /**
     @brief Get the name of the topology for this selection.
     */
    const std::string &get_topology() const;

    /**
     @brief Set the topology used for selection. The topology needs to be 
            valid for the selection mesh.
     @param value The new topology name.
     */
    void set_topology(const std::string &value);  

    /**
     @brief Returns the destination rank for this selection. The default
            version returns -1, indicating that we don't care where the
            domain is placed.
     @return The destination rank for this selection.
     */
    virtual int get_destination_rank() const;

    /**
     @brief Returns the destination rank for this selection. The default
            version returns -1, indicating that we'll number the domain.
     @return The destination domain for this selection.
     */
    virtual int get_destination_domain() const;

    /**
     @brief Returns the cell ids in this selection that are contained in the
            selected topology.
     @param n_mesh A Conduit node that contains the mesh.
     @param[out] element_ids A vector of element ids that are selected.
     */
    virtual void get_element_ids(const conduit::Node &n_mesh,
                                 std::vector<index_t> &element_ids) const = 0;

    /**
     @brief Returns a topology node that we can use from the mesh.
     @note This method throws an exception if the topology does not exist.
     @param n_mesh A Conduit node that contains the mesh.
     @return A reference to the selected topology node.
     */
    const conduit::Node &selected_topology(const conduit::Node &n_mesh) const;

    /**
     @brief Prints the selection to the stream.
     @param os The stream to which the information will be printed.
     */
    virtual void print(std::ostream &os) const = 0;

    /**
     @brief Returns whether the selection is applied to any domain.
     @return True if the selection is applied to any domain.
     */
    bool get_domain_any() const;

protected:
    /**
     @brief Returns whether the selection type is allowed to apply to any domain.
     */
    virtual bool supports_domain_any() const;

    /**
     @brief Determines whether the selection covers the whole mesh.
     @param n_mesh A Conduit conduit::Node containing the mesh.
     @return True of the mesh is whole. False otherwise.
     */
    virtual bool determine_is_whole(const conduit::Node &n_mesh) const = 0;

    static const std::string DOMAIN_KEY;
    static const std::string TOPOLOGY_KEY;

    enum {
        WHOLE_UNDETERMINED,
        WHOLE_DETERMINED_FALSE,
        WHOLE_DETERMINED_TRUE
    };

    int         whole;
    index_t     domain;
    std::string topology;
    bool        domain_any;
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/**
 @brief This class can read a set of selections and apply them to a Conduit
        node containing single or multi-domain meshes and produce a new
        Conduit node that refashions the selections into a target number of
        mesh domains. This is the serial implementation.
 */
class CONDUIT_BLUEPRINT_API Partitioner
{
public:
    static constexpr int INVALID_NODE = -1;

    /**
     @brief This struct is a Conduit/Blueprint mesh plus an ownership bool.
            The mesh pointer is always assumed to be external by default
            so we do not provide a destructor. If we want to delete it,
            call free(), which will free the mesh if we own it. The struct
            does not have a destructor on purpose.
     */
    struct CONDUIT_BLUEPRINT_API Chunk
    {
        Chunk();
        Chunk(const Node *m, bool own);
        Chunk(const Node *m, bool own, int dr, int dd);
        void free();

        const Node *mesh;
        bool        owns;
        int         destination_rank;
        int         destination_domain;
    };

    /**
     @brief Constructor.
     */
    Partitioner();

    /**
     @brief Destructor.
     */
    virtual ~Partitioner();

    /**
     @brief Initialize the partitioner using the input mesh (which could be
            multidomain) and a set of options. The options specify how we
            may be pulling apart the mesh using selections. The selections
            are allowed to be empty, in which case we're using all of the
            input mesh domains.

     @note We initialize before execute() because we may want the opportunity
           to split selections into an appropriate target number of domains.

     @param n_mesh A Conduit node representing the Blueprint mesh.
     @param options A Conduit node containing the partitioning options.

     @return True if the options were accepted or False if they contained
             an error.
     */
    bool initialize(const conduit::Node &n_mesh, const conduit::Node &options);

    /**
     @brief This method enables the partitioner to split the selections until
            we arrive at a number of selections that will yield the desired
            number of target domains.

     @note We could oversplit at some point if we're already unstructured
           if we want to load balance mesh sizes a little better.
     */
    virtual void split_selections();

    /**
     @brief Execute the partitioner so we use any options to arrive at the
            target number of domains. This may involve splitting domains,
            redistributing them (in parallel), and then combining them to
            populate the output node.

     @param[out] output The Conduit node that will receive the output meshes.
     */
    void execute(conduit::Node &output);


    /**
     @brief Given a vector of input Blueprint meshes, we combine them into 
            a single Blueprint mesh stored in output.

     @param domain The domain number being created out of the various inputs.
     @param inputs A vector of Blueprint meshes representing the input
                   meshes that will be combined.
     @param chunk_ids The global chunk id for each Blueprint mesh.
     @param[out] output The Conduit node to which the combined mesh's properties
                        will be added.

     @note This method is exposed so we can create a partitioner object and 
           combine meshes with it directly, which would be useful for development
           and unit tests. This method is serial only and will only operate on
           its inputs to generate the single combined mesh for the output node.
     */
    void combine(int domain,
                 const std::vector<const Node *> &inputs,
                 const std::vector<index_t> &chunk_ids,
                 Node &output);

    /**
     @brief Maps back fields from a repartitioned mesho onto the original mesh.

     @note This method is a standalone method and does not require calling
           Partitioner::initialize().
     */
    void map_back_fields(const conduit::Node& repart_mesh,
                         const conduit::Node& options,
                         Node& orig_mesh);

protected:

    using ChunkToVertsMap = std::unordered_map<index_t, std::vector<index_t>>;
    using DomainToChunkMap = std::unordered_map<const Node*, ChunkToVertsMap>;

    /**
     @brief Gets a set of global domain IDs. If the field "state/domain_id"
            exists, the IDs are used from there; otherwise sequentially-ordered
            domain IDs are generated that are unique to each domain on each
            processor.
     @param doms The array of domain pointers local to this processor.
     @return The corresponding global domain IDs for each domain.

     @note This method is not dependent on Partitioner::initialize()
     */
    virtual std::vector<index_t> get_global_domids(const conduit::Node& n_mesh);

    /**
     @brief Examines the selections and counts them to determine a number of
            targets that would be produced. Most selections result in a domain
            but some selections may combine into a single domain if their
            destination domain is set to the same value.
     @return The number of targets we expect to create.
     */
    virtual unsigned int count_targets() const;

    /**
     @brief Compute the total number of selections across all ranks.
     @return The total number of selections across all ranks.

     @note Reimplement in parallel
     */
    virtual long get_total_selections() const;

    /**
     @brief Obtain the target value from the options if it is present.
     @param options The options node.
     @param[out] value The target value.

     @return True if the target value was present and a good value. False if
             target value was absent or contained a bad value.

     @note Reimplemented in parallel
     */
    virtual bool options_get_target(const conduit::Node &options, unsigned int &value) const;

    /**
     @brief Get the rank and index of the largest selection. In parallel, we
            will be looking across all ranks to find the largest domains so
            those are split first.

     @note  Splitting one at a time is temporary since in parallel, it's not
            good enough. One of the reasons we split one at a time right now
            is to allow really large selections to be split more than once 
            before other selections are considered.

     @param[out] sel_rank The rank that contains the largest selection.
     @param[out] sel_index The index of the largest selection on sel_rank.

     @note Reimplemented in parallel
     */
    virtual void get_largest_selection(int &sel_rank, int &sel_index) const;

    /**
     @brief This is a factory method for creating selections based on the 
            provided options.
     @param type The name of the selection type to create.
     @return A new instance of the requested selection type.
     */
    virtual std::shared_ptr<Selection> create_selection(const std::string &type) const;

    /**
     @brief Create a selection of the type that best selects all of the 
            elements in the supplied mesh. For example, if we're given a 
            structured mesh, we'd return a logical selection that spans
            all cells in the mesh.
     @param n_mesh A Conduit node that contains the mesh we're selecting.
     @return A new selection object that selects all cells in the mesh.
     */
    std::shared_ptr<Selection> create_selection_all_elements(const conduit::Node &n_mesh) const;

    void copy_matsets(const std::string &topology,
                      const std::vector<index_t> &element_ids,
                      const conduit::Node &n_mesh,
                      conduit::Node &output) const;

    /**
     @brief Use the vertex_ids and element_ids to copy from the original
            fields to make new, restricted fields. We also add some mapping
            variables so we preserve the old vertex and element ids in the
            new field output.

     @param domain The domain associated the mesh's fields.
     @param topology The name of the topology we're using.
     @param vertex_ids The vertex ids that we're copying to the new fields.
     @param element_ids The element ids that we're copying to the new fields.
     @param n_mesh A Conduit node containing the source mesh.
     @param output A new Conduit node to contain the new mesh.
     */
    void copy_fields(index_t domain,
                     const std::string &topology,
                     const std::vector<index_t> &vertex_ids,
                     const std::vector<index_t> &element_ids,
                     const conduit::Node &n_mesh,
                     conduit::Node &output) const;

    void copy_field(const conduit::Node &n_field,
                    const std::vector<index_t> &ids,
                    Node &n_output_fields) const;

    void get_vertex_ids_for_element_ids(const conduit::Node &n_topo,
             const std::vector<index_t> &element_ids,
             std::set<index_t> &vertex_ids) const;

    /**
     @brief Extract the idx'th selection from the input mesh and return a
            new Node containing the extracted chunk.

     @param idx The selection to extract. This must be a valid selection index.
     @param n_mesh A Conduit node representing the mesh to which we're
                   applying the selection.
     @return A node containing the extracted chunk. The chunks will need to
             be freed by the caller.
     */
    conduit::Node *extract(size_t idx,
                           const conduit::Node &n_mesh,
                           std::vector<index_t>& vertex_ids_out) const;

    void create_new_uniform_coordset(const conduit::Node &n_coordset,
             const index_t start[3],
             const index_t end[3],
             conduit::Node &n_new_coordset) const;

    void create_new_rectilinear_coordset(const conduit::Node &n_coordset,
             const index_t start[3],
             const index_t end[3],
             conduit::Node &n_new_coordset) const;

    void create_new_explicit_coordset(const conduit::Node &n_coordset,
             const std::vector<index_t> &vertex_ids,
             conduit::Node &n_new_coordset) const;

    void create_new_uniform_topo(const conduit::Node &n_topo,
             const std::string &csname,
             const index_t start[3],
             conduit::Node &n_new_topo) const;

    void create_new_rectilinear_topo(const conduit::Node &n_topo,
             const std::string &csname,
             const index_t start[3],
             conduit::Node &n_new_topo) const;

    void create_new_structured_topo(const conduit::Node &n_topo,
             const std::string &csname,
             const index_t start[3],
             const index_t end[3],
             conduit::Node &n_new_topo) const;

    /**
     @brief Creates a new unstructured topology from a subset of the 
            input topology. Any topologies that are not unstructured are
            currently converted to unstructured.

     @param n_topo A Conduit node containing source topology.
     @param csname The name of the coordset to use in the new topology.
     @param originalCoordsetLength The original coordset length.
     @param element_ids The element ids to include from the source topology.
     @param vertex_ids The vertex ids that are used from the source topology's coordset.
     @param n_new_topo A Conduit node that will contain the new topology.
     */
    void create_new_unstructured_topo(const conduit::Node &n_topo,
             const std::string &csname,
             index_t originalCoordsetLength,
             const std::vector<index_t> &element_ids,
             const std::vector<index_t> &vertex_ids,
             conduit::Node &n_new_topo) const;

    /**
     @brief Creates a new unstructured topology from a subset of the 
            input unstructured topology.

     @param n_topo A Conduit node containing source topology.
     @param csname The name of the coordset to use in the new topology.
     @param originalCoordsetLength The original coordset length.
     @param element_ids The element ids to include from the source topology.
     @param vertex_ids The vertex ids that are used from the source topology's coordset.
     @param n_new_topo A Conduit node that will contain the new topology.
     */
    void unstructured_topo_from_unstructured(const conduit::Node &n_topo,
             const std::string &csname,
             index_t originalCoordsetLength,
             const std::vector<index_t> &element_ids,
             const std::vector<index_t> &vertex_ids,
             conduit::Node &n_new_topo) const;

    /**
     @brief Creates a new unstructured topology from a subset of the 
            input unstructured topology.

     @param n_topo A Conduit node containing source topology.
     @param element_ids The element ids to include from the source topology.
     @param old2new A map of old vertex ids to new vertex ids.
     @param n_new_topo A Conduit node that will contain the new topology.
     */
    void unstructured_topo_from_polygonal(const conduit::Node &n_topo,
             const std::vector<index_t> &element_ids,
             const std::vector<index_t> &old2new,
             conduit::Node &n_new_topo) const;

    /**
     @brief Creates a new unstructured topology from a subset of the 
            input unstructured topology.

     @param n_topo A Conduit node containing source topology.
     @param element_ids The element ids to include from the source topology.
     @param old2new A map of old vertex ids to new vertex ids.
     @param n_new_topo A Conduit node that will contain the new topology.
     */
    void unstructured_topo_from_polyhedral(const conduit::Node &n_topo,
             const std::vector<index_t> &element_ids,
             const std::vector<index_t> &old2new,
             conduit::Node &n_new_topo) const;

    /**
     @brief Creates a new unstructured topology from a subset of the 
            input unstructured topology.

     @param n_topo A Conduit node containing source topology.
     @param element_ids The element ids to include from the source topology.
     @param old2new A map of old vertex ids to new vertex ids.
     @param n_new_topo A Conduit node that will contain the new topology.
     */
    void unstructured_topo_from_stream(const conduit::Node &n_topo,
             const std::vector<index_t> &element_ids,
             const std::vector<index_t> &old2new,
             conduit::Node &n_new_topo) const;

    /**
     @brief Creates a new unstructured topology from a subset of the 
            input unstructured topology.

     @param n_topo A Conduit node containing source topology.
     @param element_ids The element ids to include from the source topology.
     @param old2new A map of old vertex ids to new vertex ids.
     @param n_new_topo A Conduit node that will contain the new topology.
     */
    void unstructured_topo_from_mixed(const conduit::Node &n_topo,
             const std::vector<index_t> &element_ids,
             const std::vector<index_t> &old2new,
             conduit::Node &n_new_topo) const;

    /**
     @brief Make a shallow copy of the input mesh node but make it have
            its own fields so we can add original vertex and element maps.
            All other fields are shallow copied.
     @param idx The index of the selection whose data we're wrapping.
     @param n_mesh The input mesh we're wrapping.
     @return A new node containing the wrapped data.
     */
    conduit::Node *wrap(size_t idx, const conduit::Node &n_mesh) const;

    /**
     @brief Given a set of input meshes which may have various topologies,
            recommend a topology that can be used to capture the combined
            meshes in a single output.

     @note If the meshes contain multiple topologies then it probably
           makes sense to recommend unstructured.

     @param inputs A vector of Blueprint mesh node pointers to consider.

     @return The recommended topology type that can represent the input
             meshes as a single mesh once they are combined.
     */
    std::string recommended_topology(const std::vector<const Node *> &inputs, 
        const std::string &topo_name) const;

    /**
     @brief Given a set of inputs that are predetermined to fit together
            into a logically structured output, perform the actual
            recombination to yield a single mesh (uniform, rectilinear, ...)
            in the output.

     @param inputs A vector of Blueprint mesh nodes to combine.
     @param output The Conduit node into which the combined mesh output
                   will be added.
     @return True on success, false on failure. False indicates that we should fall back
            on combining as unstructured.
     */
    bool combine_as_structured(const std::string &topo_name,
        const std::string &rt,
        const std::vector<const Node *> &inputs,
        Node &output);

    /**
     @brief Given a set of inputs that are of various types, assemble them
            into a single output mesh with unstructured topology. This
            method combines like-named coordsets and topologies.

     @param inputs A vector of Blueprint mesh nodes to combine.
     @param output The Conduit node into which the combined mesh output
                   will be added.
     */
    void combine_as_unstructured(const std::string &topo_name,
        const std::vector<std::pair<std::string, std::vector<const Node*>>> &topo_groups,
        const std::vector<std::pair<std::string, std::vector<const Node*>>> &coordset_groups,
        Node &output);

    /**
     @brief Given a local set of chunks, figure out starting domain index
            that will be used when numbering domains on a rank. We figure
            out which ranks get domains and this is the scan of the domain
            numbers.
     
     @return The starting domain index on this rank.
     */
    virtual unsigned int starting_index(const std::vector<Chunk> &chunks);

    /**
     @brief Assign the chunks on this rank a destination rank to which it will
            be transported as well as a destination domain that indices which
            chunks will be combined into the final domains.

     @note All chunks that get the same dest_domain must also get the same
           dest_rank since dest_rank is the MPI rank that will do the work
           of combining the chunks it gets. Also, this method nominates
           ranks as those who will receive chunks. If there are 4 target
           chunks then when we run in parallel, 4 ranks will get a domain.
           This will be overridden for parallel.

     @param chunks A vector of input chunks that we are mapping to ranks
                   and domains.
     @param[out] dest_ranks The destination ranks that get each input chunk.
     @param[out] dest_domain The destination domain to which a chunk is assigned.
     @param offsets[out]     A vector of MPI_Comm_size that contains the index at
                             which a rank's data begins in dest_rank, dest_domain.
                             It contains a single 0 in serial.
     @note Reimplemented in parallel
     */
    virtual void map_chunks(const std::vector<Chunk> &chunks,
                            std::vector<int> &dest_ranks,
                            std::vector<int> &dest_domain,
                            std::vector<int> &offsets);

    /**
     @brief Generates initial intermediate adjsets for each chunk, based on the
            adjsets in the original domains.

     @param chunk_assoc_adjset An array of nodes pointing to corresponding
                               pre-load balance domain adjsets for each chunk.
     @param[out] adjset_data An array of nodes containing adjset data for each
                             chunk local to this rank.
     */
    void init_chunk_adjsets(const std::vector<const conduit::Node*>& chunk_assoc_adjset,
                            std::vector<conduit::Node*>& adjset_data);

    /**
     @brief Generates a map from adjset vertex ids to their containing chunks.
            This map is generated per-adjset and is stored in CSR format, and
            is called by build_interdomain_adjsets() to construct splits of the
            original adjsets.

     @param chunk_offsets The vector containing starting chunk indices for each
                          rank.
     @param chunks A map of pre-partition domains to the set of chunk ids it is
                   decomposed into, as well as the associated vertex maps for
                   each chunk.
     @param domain_map A map of domain ids to the corresponding Conduit node
                       containing that domain's data.
     @param[out] adjset_chunk_maps An array of nodes containing mappings from
                                   shared vertices in adjsets to output chunks.
     @note Reimplemented in parallel
     */
    virtual void get_prelb_adjset_maps(const std::vector<int>& chunk_offsets,
                                       const DomainToChunkMap& chunks,
                                       const std::map<index_t, const Node*>& domain_map,
                                       std::vector<Node>& adjset_chunk_maps);

    /**
     @brief Splits original adjsets into new adjsets for each constituent chunk
            of all domains in the problem.

     @param chunk_offsets The vector containing starting chunk indices for each
                          rank.
     @param chunks A map of pre-partition domains to the set of chunk ids it is
                   decomposed into, as well as the associated vertex maps for
                   each chunk.
     @param domain_map A map of domain ids to the corresponding Conduit node
                       containing that domain's data.
     @param[out] adjset_data An array of nodes containing adjset data for each
                             chunk local to this rank.
     */
    void build_interdomain_adjsets(const std::vector<int>& chunk_offsets,
                                   const DomainToChunkMap& chunks,
                                   const std::map<index_t, const Node*>& domain_map,
                                   std::vector<conduit::Node*>& adjset_data);

    /**
     @brief Generates shared vertex information for chunks within the same
            pre-partitioned domain, and adds new adjset groups to all adjsets
            in any applicable chunk. This should be called after the original
            adjsets have been split for each chunk.

     @param chunk_offsets The vector containing starting chunk indices for each
                          rank.
     @param chunks A map of pre-partition domains to the set of local chunk ids
                   it is decomposed into, as well as the associated vertex maps
                   for each chunk.
     @param[out] adjset_data An array of nodes containing adjset data for each
                             chunk local to this rank.
     */
    void build_intradomain_adjsets(const std::vector<int>& chunk_offsets,
                                   const DomainToChunkMap& chunks,
                                   std::vector<conduit::Node*>& adjset_data);

    /**
     @brief Communicates the input chunks to their respective destination ranks
            and passes out the set of chunks that this rank will operate on in
            the chunks_to_assemble vector.

     @param chunks The vector of input chunks that may be redistributed to
                   different ranks.
     @param dest_rank A vector of integers containing the destination ranks of
                      each chunk.
     @param dest_domain The global numbering of each input chunk.
     @param offsets     A vector of MPI_Comm_size that contains the index at
                        which a rank's data begins in dest_rank, dest_domain.
                        It contains a single 0 in serial.
     @param[out] chunks_to_assemble The vector of chunks that this rank will
                                    combine and set into the output.
     @param[out] chunks_to_assemble_domains The global domain numbering of each
                                            chunk in chunks_to_assemble. Like-
                                            numbered chunks will be combined
                                            into a single output domain.
     @param[out] chunks_to_assemble_gids The global chunk numbering of each
                                         chunk in chunks_to_assemble.
     @note Reimplemented in parallel
     */
    virtual void communicate_chunks(const std::vector<Chunk> &chunks,
                                    const std::vector<int> &dest_rank,
                                    const std::vector<int> &dest_domain,
                                    const std::vector<int> &offsets,
                                    std::vector<Chunk> &chunks_to_assemble,
                                    std::vector<int> &chunks_to_assemble_domains,
                                    std::vector<int> &chunks_to_assemble_gids);

    /**
     @brief During the field back-map, communicates packed field data to the
            correct domain homes for the original mesh.
     @param packed_fields   A map of original domain IDs to Conduit nodes
                            containing sliced field data belonging to that
                            domain.
     @note Reimplemented in parallel
     @note This method is not dependent on Partitioner::initialize()
     */
    virtual void communicate_mapback(std::unordered_map<index_t, Node>& packed_fields);

    /**
     @brief During the field back-map, communicates global vertex id information
            to any ranks that need it for constructing the map-back vertex
            correspondence.
     @param remap_to_local_doms For each remapped domain on this rank, the set
                                of original domains constituting it.
     @param orig_dom_gvids  The map of global vertex IDs for each original
                            domain on this processor. Extra global vertex IDs
                            will be inserted into this map.
     @note Reimplemented in parallel
     @note This method is not dependent on Partitioner::initialize()
     */
    virtual void synchronize_gvids(
        const std::vector<std::vector<index_t>>& remap_to_local_doms,
        std::map<index_t, std::vector<index_t>>& orig_dom_gvids);

    /**
     @brief Return the name of the field that contains original vertex ids.

     @return The name of the field to use for original vertex ids.
     */
    const std::string &original_vertex_ids_name() const
    {
        return original_vertex_ids;
    }

    /**
     @brief Return the name of the field that contains original element ids.

     @return The name of the field to use for original element ids.
     */
    const std::string &original_element_ids_name() const
    {
        return original_element_ids;
    }

    int rank, size;
    unsigned int target;
    std::vector<const Node *>                meshes;
    std::vector<std::shared_ptr<Selection> > selections;
    std::vector<std::string>                 selected_fields;
    bool                                     mapping;
    bool                                     build_adjsets;
    double                                   merge_tolerance;
    std::string                              original_vertex_ids;
    std::string                              original_element_ids;
};

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
