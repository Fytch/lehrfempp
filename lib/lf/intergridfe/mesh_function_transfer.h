#ifndef LF_INTERGRIDFE_MESH_FUNCTION_TRANSFER_H
#define LF_INTERGRIDFE_MESH_FUNCTION_TRANSFER_H

#include <lf/assemble/dofhandler.h>
#include <lf/refinement/mesh_hierarchy.h>
#include <lf/uscalfe/uscalfe.h>
#include <Eigen/Dense>
#include <type_traits>

namespace lf::intergridfe {

/**
 * @brief Interpolate a vector of DOFs on a finer mesh
 * @tparam SCALAR_COEFF The scalar of the coefficient vector
 * @tparam FES_COARSE The FE space on the coarse mesh
 * @tparam FES_FINE The FE space on the fine mesh
 * @param mh A reference to the MeshHierarchy containing the underlying meshes
 * @param fespace_coarse The FE space on the coarse mesh
 * @param fespace_fine The FE space on the fine mesh
 * @param dofs_coarse A basis function coefficient vector on the coarse mesh
 * @param level The level of the coarse mesh
 * @returns An interpolated vector of basis function coefficients on the fine
 * mesh
 */
template <typename SCALAR_COEFF, typename FES_COARSE, typename FES_FINE>
[[nodiscard]] Eigen::Matrix<SCALAR_COEFF, Eigen::Dynamic, 1> prolongate(
    const lf::refinement::MeshHierarchy &mh,
    std::shared_ptr<const FES_COARSE> fespace_coarse,
    std::shared_ptr<const FES_FINE> fespace_fine,
    const Eigen::Matrix<SCALAR_COEFF, Eigen::Dynamic, 1> &dofs_coarse,
    lf::base::size_type level) {
  // Assert that the FES_* are actually FE spaces
  using scalar_fe_coarse_t = typename FES_COARSE::Scalar;
  using scalar_fe_fine_t = typename FES_FINE::Scalar;
  static_assert(
      std::is_convertible_v<
          FES_COARSE, lf::uscalfe::UniformScalarFESpace<scalar_fe_coarse_t>>,
      "Invalid coarse FE space provided");
  static_assert(
      std::is_convertible_v<
          FES_FINE, lf::uscalfe::UniformScalarFESpace<scalar_fe_fine_t>>,
      "Invalid fine FE space provided");
  // Obtain the dofhandlers from the fe spaces
  const lf::assemble::DofHandler &dofh_coarse{fespace_coarse->LocGlobMap()};
  const lf::assemble::DofHandler &dofh_fine{fespace_fine->LocGlobMap()};
  const lf::base::size_type N_coarse = dofh_coarse.NumDofs();
  const lf::base::size_type N_fine = dofh_fine.NumDofs();
  // Assert correctness of inputs
  LF_ASSERT_MSG(level < mh.NumLevels() - 1,
                "level must not point to the finest mesh in the hierarchy");
  LF_ASSERT_MSG(
      dofs_coarse.size() >= N_coarse,
      "Too few basis function coefficients provided for coarse FE space");
  // Construct a mesh function to simplify the point evaluations
  const lf::uscalfe::MeshFunctionFE mf_coarse(fespace_coarse, dofs_coarse);
  // Initialize the dof vector on the fine mesh
  Eigen::Matrix<SCALAR_COEFF, Eigen::Dynamic, 1> dofs_fine =
      Eigen::Matrix<SCALAR_COEFF, Eigen::Dynamic, 1>::Zero(N_fine);
  // Iterate over all entities of the fine mesh and compute the dof values
  const auto mesh_fine = mh.getMesh(level + 1);
  const auto &pinfos = mh.ParentInfos(level + 1, 0);
  for (const auto child : mesh_fine->Entities(0)) {
    const lf::base::size_type child_idx = mesh_fine->Index(*child);
    const auto rel_geom = mh.GeometryInParent(level + 1, *child);
    const auto layout = fespace_fine->ShapeFunctionLayout(child->RefEl());
    // Compute the value of the coarse mesh function at the evaluation nodes
    const auto eval_nodes = rel_geom->Global(layout->EvaluationNodes());
    const auto parent = pinfos[child_idx].parent_ptr;
    const auto nodal_values = mf_coarse(*parent, eval_nodes);
    // Convert the nodal values to dofs
    using scalar_t = typename decltype(nodal_values)::value_type;
    const Eigen::Map<const Eigen::Matrix<scalar_t, Eigen::Dynamic, 1>>
        nodal_values_map(nodal_values.data(), nodal_values.size());
    const auto dofs = layout->NodalValuesToDofs(nodal_values_map);
    // Set the dofs in the dof vector on the fine mesh
    const auto dofidxs = dofh_fine.GlobalDofIndices(*child);
    for (lf::base::size_type i = 0; i < dofidxs.size(); ++i) {
      dofs_fine[dofidxs[i]] = dofs[i];
    }
  }
  return dofs_fine;
}

}  // namespace lf::intergridfe

#endif  // LF_INTERGRIDFE_MESH_FUNCTION_TRANSFER_H
