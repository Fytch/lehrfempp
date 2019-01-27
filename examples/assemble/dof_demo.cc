/**
 * @file
 * @brief Outputs location of global shape functions as managed by a Dofhandler
 * @author Ralf Hiptmair
 * @date   October 2018
 * @copyright MIT License
 */

#include <boost/program_options.hpp>

#include <lf/assemble/assemble.h>
#include <lf/geometry/geometry.h>
#include <lf/mesh/hybrid2d/hybrid2d.h>
#include "lf/assemble/assemble.h"
#include "lf/mesh/test_utils/test_meshes.h"
#include "lf/mesh/utils/utils.h"

/**
 * @brief output of information stored in DofHandler
 * @parm dofh reference to a DofHandler object
 * @sa std::ostream &lf::assmeble::operator<<(std::ostream &o, const DofHandler
 * &dof_handler)
 */
void printDofInfo(const lf::assemble::DofHandler &dofh) {
  // Obtain pointer to the underlying mesh
  auto mesh = dofh.Mesh();
  // Number of degrees of freedom managed by the DofHandler object
  const lf::assemble::size_type N_dofs(dofh.NoDofs());
  std::cout << "DofHandler(" << dofh.NoDofs() << " dofs):" << std::endl;
  // Output information about dofs for entities of all co-dimensions
  for (lf::base::dim_t codim = 0; codim <= mesh->DimMesh(); codim++) {
    // Visit all entities of a codimension codim
    for (const lf::mesh::Entity &e : mesh->Entities(codim)) {
      // Fetch unique index of current entity supplied by mesh object
      const lf::base::glb_idx_t e_idx = mesh->Index(e);
      // Number of shape functions covering current entity
      const lf::assemble::size_type no_dofs(dofh.NoLocalDofs(e));
      // Obtain global indices of those shape functions ...
      lf::base::RandomAccessRange<const lf::assemble::gdof_idx_t> dofarray{
          dofh.GlobalDofIndices(e)};
      // and print them
      std::cout << e << ' ' << e_idx << ": " << no_dofs << " dofs = [";
      for (int loc_dof_idx = 0; loc_dof_idx < no_dofs; ++loc_dof_idx) {
        std::cout << dofarray[loc_dof_idx] << ' ';
      }
      std::cout << ']';
      // Also output indices of interior shape functions
      lf::base::RandomAccessRange<const lf::assemble::gdof_idx_t> intdofarray{
          dofh.InteriorGlobalDofIndices(e)};
      std::cout << " int = [";
      for (lf::assemble::gdof_idx_t int_dof : intdofarray) {
        std::cout << int_dof << ' ';
      }
      std::cout << ']' << std::endl;
    }
  }
  // List entities associated with the dofs managed by the current
  // DofHandler object
  for (lf::assemble::gdof_idx_t dof_idx = 0; dof_idx < N_dofs; dof_idx++) {
    const lf::mesh::Entity &e(dofh.Entity(dof_idx));
    std::cout << "dof " << dof_idx << " -> " << e << " " << mesh->Index(e)
              << std::endl;
  }
}  // end function printDofInfo

int main(int argc, char **argv) {
  // The following code is modeled after the example from
  // https://theboostcpplibraries.com/boost.program_options
  // and defines allowed command line arguments:
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
  ("help,h", "--ndof_node <N> --ndof_edge <N> --ndof_tria <N> --ndof_quad <N>")
  ("ndof_node,n", po::value<int>()->default_value(1), "No of dofs on nodes")
  ("ndof_edge,e", po::value<int>()->default_value(2), "No of dofs on edges")
  ("ndof_tria,t", po::value<int>()->default_value(1), "No of dofs on triangles")
  ("ndof_quad,q", po::value<int>()->default_value(4), "Mp of dofs on quadrilaterals");
  // clang-format on
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  if (vm.count("help") > 0) {
    std::cout << desc << std::endl;
  } else {
    // Retrieve number of degrees of freedom for each entity type from command
    // line arguments
    lf::base::size_type ndof_node = 1;
    if (vm.count("ndof_node") > 0) {
      ndof_node = vm["ndof_node"].as<int>();
    }
    lf::base::size_type ndof_edge = 2;
    if (vm.count("ndof_edge") > 0) {
      ndof_edge = vm["ndof_edge"].as<int>();
    }
    lf::base::size_type ndof_tria = 1;
    if (vm.count("ndof_tria") > 0) {
      ndof_tria = vm["ndof_tria"].as<int>();
    }
    lf::base::size_type ndof_quad = 4;
    if (vm.count("ndof_quad") > 0) {
      ndof_quad = vm["ndof_quad"].as<int>();
    }
    std::cout << "LehrFEM++ demo: assignment of global shape functions"
              << std::endl;
    std::cout << "#dof/vertex = " << ndof_node << std::endl;
    std::cout << "#dof/edge = " << ndof_edge << std::endl;
    std::cout << "#dof/triangle = " << ndof_tria << std::endl;
    std::cout << "#dof/quadrilateral = " << ndof_quad << std::endl;

    // Build a mesh comprising two cells
    std::shared_ptr<lf::mesh::Mesh> mesh_p =
        lf::mesh::test_utils::GenerateHybrid2DTestMesh(2);
    // Output information about the mesh
    lf::mesh::utils::printinfo_ctrl = 100;
    lf::mesh::Entity::output_ctrl_ = 0;
    lf::mesh::utils::PrintInfo(*mesh_p, std::cout);

    // Create a dof handler object describing a uniform distribution
    // of shape functions
    lf::assemble::UniformFEDofHandler dof_handler(
        mesh_p, {{lf::base::RefEl::kPoint(), ndof_node},
                 {lf::base::RefEl::kSegment(), ndof_edge},
                 {lf::base::RefEl::kTria(), ndof_tria},
                 {lf::base::RefEl::kQuad(), ndof_quad}});
    lf::assemble::DofHandler::output_ctrl_ = 30;
    std::cout << dof_handler << std::endl;
    std::cout << "============================================================"
              << std::endl;
    printDofInfo(dof_handler);
  }
  return 0L;
}  // end main
