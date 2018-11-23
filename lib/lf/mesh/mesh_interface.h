
#ifndef __b86b0c9cb0fd48da931a1f24421b8842
#define __b86b0c9cb0fd48da931a1f24421b8842

#include <lf/base/base.h>

#include "entity.h"

namespace lf::mesh {
/**
 * @brief Abstract interface for objects representing a single mesh
 *
 * This abstract base class desccribes the basic functionality of objects
 * that manage single-level conforming finite element meshes. These objects
 * essentially boil down to containers for mesh entities of different
 * co-dimensions. Thus they allow sequential traversal of these entities.
 *
 * Another important functionality concerns the management of entity indices,
 * which have to provide a consecutive numbering of entities of a specific
 * co-dimension starting from zero.
 */
class Mesh {
 protected:
  Mesh() = default;
  Mesh(const Mesh&) = default;
  Mesh(Mesh&&) = default;
  Mesh& operator=(const Mesh&) = default;
  Mesh& operator=(Mesh&&) = default;

 public:
  /** Auxiliary types */
  using size_type = lf::base::size_type;
  using dim_t = lf::base::dim_t;
  /**
   * @brief The dimension of the manifold described by the mesh, or
   *        equivalently the maximum dimension of the reference elements
   *        present in the mesh.
   */
  virtual char DimMesh() const = 0;

  /**
   * @brief The dimension of the Euclidean space in which the mesh is
   *        embedded.
   */
  virtual char DimWorld() const = 0;

  /**
   * @brief All entities of a given codimension.
   * @param codim The codimension of the entities that should be returned.
   * @return A base::ForwardRange that can be used to traverse the entities.
   *
   * @sa Entity
   *
   * Principal access method for entities distinguished only by their
   * co-dimension Hence, all cells of a mesh are covered by the range returned
   * when giving co-dimension 0, regardless of their concrete shape.
   */
  virtual base::ForwardRange<const Entity> Entities(char codim) const = 0;

  /**
   * @brief The number of Entities that have the given codimension.
   * @param codim The codimension of the entities that should be counted.
   * @return That number of entities that have the given codimension.
   */
  virtual size_type Size(char codim) const = 0;

  /**
   * @brief Tells number of entities of a particular topological/geometric type
   * @param ref_el_type topological/geometric type
   * @return number of entities of that type
   */
  virtual size_type NumEntities(lf::base::RefEl ref_el_type) const = 0; 
  
  /**
   * @brief Acess to the index of a mesh entity of any co-dimension
   * @param e Entity whose index is requested
   * @return index ranging from 0 to no. of entities of the same co-dimension-1
   *
   * It is a strict convention in LehrFEM++ that all entities of the same
   * co-dimension belonging to a mesh are endowed with an integer index. These
   * indices are guaranteed to be contiguous and to range from 0 to
   * `Size(codim)-1`.
   * @note The index of a mesh entity is NOT related to its position in the
   * range returned by the Entities() method.
   */
  virtual size_type Index(const Entity& e) const = 0;

  /**
   * @brief Method for accessing an entity through its index
   * @param codim codimension of the entity. Remember that indices are supposed
   *        to be unique and contiguous for a given co-dimension
   * @param index an integer between 0 and number of entities of the given
   * co-dimension -1. It passes the index.
   *
   * Based on the bijectition between entities of a given co-dimension and an
   * integer range.
   *
   * @note O(1) access complexity due to table lookup.
   */
  virtual const mesh::Entity* EntityByIndex(dim_t codim,
                                            base::glb_idx_t index) const = 0;

  /**
   * @brief Check if the given entity is a part of this mesh.
   * @param e The entity that should be checked.
   * @return true if the entity lies in this mesh.
   */
  virtual bool Contains(const Entity& e) const = 0;

  /**
   * @brief virtual destructor
   */
  virtual ~Mesh() = default;
};

}  // namespace lf::mesh

#endif  // __b86b0c9cb0fd48da931a1f24421b8842
