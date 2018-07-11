/**
 * @file refinement.cc
 * @brief implementation of splitting of reference entities according to refinement 
 *         patterns  
 */

#include "refinement.h"

namespace lf::refinement {

  // Implementation for RefinemeentPattern

lf::base::size_type Hybrid2DRefinementPattern::noChildren(void) const {
    // Depending on the type of cell do something different
  switch (ref_el_) {
  case lf::base::RefEl::kPoint(): {
    switch (ref_pat_) {
    case RefPat::rp_nil: { return 0; }
    case RefPat::rp_copy: { return 1; }
    default: {
      LF_VERIFY_MSG(false,"Illegal refinement pattern for point");
    }} // end swtich ref_pat_
    break;
  } // end case of a simple point
  case lf::base::RefEl::kSegment(): {
    switch (ref_pat_) {
    case RefPat::rp_nil: { return 0; }
    case RefPat::rp_copy: { return 1; }
    case RefPat::rp_split: { return 2; }
    default: {
      LF_VERIFY_MSG(false,"refinement pattern " << (int)ref_pat_
		    << "not implemented for edge!");
      break;
    }
    } // end switch ref_pat_
    break;
  } // end case of an edge
  case lf::base::RefEl::kTria(): {
    switch (ref_pat_) {
    case RefPat::rp_nil: { return 0; }
    case (int)RefPat::rp_copy: { return 1; }
    case (int)RefPat::rp_bisect: { return 2; } 
    case (int)RefPat::rp_trisect: { return 3; }
    case (int)RefPat::rp_trisect_left: { return 3; }
    case (int)RefPat::rp_quadsect: { return 4; }
    case (int)RefPat::rp_regular: { return 4; } 
    case (int)RefPat::rp_barycentric: { return 6; }
    default: {
      LF_VERIFY_MSG(false,"refinement pattern " << (int)ref_pat_
		  << "not implemented for triangle!");
      break;
    }
    } // end switch ref_pat_
    break;
  } // end case of a triangle
  case lf::base::RefEl::kQuad(): {
    switch (ref_pat_) {
    case RefPat::rp_nil: { return 0; }
    case (int)RefPat::rp_copy: { return 1; } 
    case (int)RefPat::rp_trisect: { return 3; }
    case (int)RefPat::rp_quadsect: { return 4; }
    case (int)RefPat::rp_bisect: 
    case (int)RefPat::rp_split: { return 2; }
    case (int)RefPat::rp_threeedge: { return 4; } 
    case (int)RefPat::rp_barycentric: 
    case (int)RefPat::rp_regular: { return 4; }
    default: {
      LF_VERIFY_MSG(false,"refinement pattern " << (int)ref_pat_
		  << "not implemented for quadrilateral!");
      break;
    }
    } // end switch ref_pat_
    break;
  } // end case of a quadrilateral
  } // end switch cell type 
  return 0;
}

std::vector<Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic>>
Hybrid2DRefinementPattern::ChildPolygons(void) const {
  std::vector<Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic>> child_poly {};
  // Lattice point coordinates
  const int lt_half = lattice_const_/2;
  const int lt_third = lattice_const_/3;
  const int lt_one = lattice_const_;
  // Depending on the type of cell do something different
  switch (ref_el_) {
  case lf::base::RefEl::kPoint(): {
    child_poly.emplace_back(0,1);
    break;
  } // end case of a simple point
  case lf::base::RefEl::kSegment(): {
    switch (ref_pat_) {
    case RefPat::rp_nil: {
      break;
    }
    case RefPat::rp_copy: {
      Eigen::Matrix<int,1,2> seg_ref_coord; seg_ref_coord << 0,lt_one;
      child_poly.push_back(seg_ref_coord);
      break;
    }
    case RefPat::rp_split: {
      Eigen::Matrix<int,1,2> part_ref_coord;
      part_ref_coord << 0,lt_half;      child_poly.push_back(part_ref_coord);
      part_ref_coord << lt_half,lt_one; child_poly.push_back(part_ref_coord);
      break;
    }
    default: {
      LF_VERIFY_MSG(false,"refinement pattern " << (int)ref_pat_
		  << "not implemented for edge!");
      break;
    }
    } // end switch ref_pat_
    break;
  } // end case of an edge
  case lf::base::RefEl::kTria(): {
    // Lattice coordinates of special points in the triangle
    // Here we use knowledge about the conventions underlying node
    // and edge numbering in a triangle as defined in RefEl.
    // This information could also be retrieved from the reference element.
    Eigen::MatrixXi lt_node_coords(2,3);
    lt_node_coords << 0,lt_one,0,0,0,lt_one;
    Eigen::MatrixXi lt_midpoint_coords(2,3);
    lt_midpoint_coords << lt_half,lt_half,0,0,lt_half,lt_half;

    // Remap local indices according to anchor values
    const int mod_0 = (0+anchor_)%3;
    const int mod_1 = (1+anchor_)%3;
    const int mod_2 = (2+anchor_)%3;

    // For temporary storage of triangular lattice polygon
    Eigen::MatrixXi child_coords(2,3);
    
    switch (ref_pat_) {
    case RefPat::rp_nil: {
      break;
    }
    case (int)RefPat::rp_copy: {
      child_coords = lt_node_coords;
      child_poly.push_back(child_coords);
      break;
    }
    case (int)RefPat::rp_bisect: {
    LF_VERIFY_MSG(anchor_set_,
		  "Anchor must be set for bisection refinement of triangle");
      // Splitting a triangle in two by bisecting anchor edge
      child_coords.col(0) = lt_node_coords.col(mod_0);
      child_coords.col(1) = lt_midpoint_coords.col(mod_0);
      child_coords.col(2) = lt_node_coords.col(mod_2);
      child_poly.push_back(child_coords);
      
      child_coords.col(0) = lt_node_coords.col(mod_1);
      child_coords.col(1) = lt_midpoint_coords.col(mod_0);
      child_coords.col(2) = lt_node_coords.col(mod_2);
      child_poly.push_back(child_coords);
      break;
    }
    case (int)RefPat::rp_trisect: {
    LF_VERIFY_MSG(anchor_set_,
		  "Anchor must be set for trisection refinement of triangle");
      // Bisect through anchor edge first and then bisect through
      // edge with the next larger index (mod 3); creates three
      // child triangles.
	child_coords.col(0) = lt_node_coords.col(mod_0);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_node_coords.col(mod_2);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_1);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_1);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_2);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	child_poly.push_back(child_coords);
      break;
    }
    case (int)RefPat::rp_trisect_left: {
      LF_VERIFY_MSG(anchor_set_,
		    "Anchor must be set for trisection refinement of triangle");

      // Bisect through anchor edge first and then bisect through
      // edge with the next smaller index (mod 3); creates three
      // child triangles.
	child_coords.col(0) = lt_node_coords.col(mod_0);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_1);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_node_coords.col(mod_2);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_2);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	child_poly.push_back(child_coords);
      break;
    }
    case (int)RefPat::rp_quadsect: {
    LF_VERIFY_MSG(anchor_set_,
		  "Anchor must be set for quadsection refinement of triangle");
      // Bisect through the anchor edge first and then
      // through the two remaining edges; creates four child
      // triangles; every edge is split.
	child_coords.col(0) = lt_node_coords.col(mod_0);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_1);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_1);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_2);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_1);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(mod_2);
	child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	child_poly.push_back(child_coords);
      break;
    }
    case (int)RefPat::rp_regular: {
      // Split triangle into four small  congruent triangles
	child_coords.col(0) = lt_node_coords.col(0);
	child_coords.col(1) = lt_midpoint_coords.col(0);
	child_coords.col(2) = lt_midpoint_coords.col(2);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(1);
	child_coords.col(1) = lt_midpoint_coords.col(0);
	child_coords.col(2) = lt_midpoint_coords.col(1);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(2);
	child_coords.col(1) = lt_midpoint_coords.col(2);
	child_coords.col(2) = lt_midpoint_coords.col(1);
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_midpoint_coords.col(0);
	child_coords.col(1) = lt_midpoint_coords.col(1);
	child_coords.col(2) = lt_midpoint_coords.col(2);
	child_poly.push_back(child_coords);
      break;
    }
    case (int)RefPat::rp_barycentric: {
      //  Split triangle into 5 smaller triangles by connecting
      // the center of gravity with the vertices and the midpoints
      // of the edges.
      
      // Obtain coordinates of center of gravity
      Eigen::Vector2i lt_baryc_coords = Eigen::Vector2i({lt_third,lt_third});

	child_coords.col(0) = lt_node_coords.col(0);
	child_coords.col(1) = lt_midpoint_coords.col(0);
	child_coords.col(2) = lt_baryc_coords;
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(1);
	child_coords.col(1) = lt_midpoint_coords.col(0);
	child_coords.col(2) = lt_baryc_coords;
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(1);
	child_coords.col(1) = lt_midpoint_coords.col(1);
	child_coords.col(2) = lt_baryc_coords;
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(2);
	child_coords.col(1) = lt_midpoint_coords.col(1);
	child_coords.col(2) = lt_baryc_coords;
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(2);
	child_coords.col(1) = lt_midpoint_coords.col(2);
	child_coords.col(2) = lt_baryc_coords;
	child_poly.push_back(child_coords);

	child_coords.col(0) = lt_node_coords.col(0);
	child_coords.col(1) = lt_midpoint_coords.col(2);
	child_coords.col(2) = lt_baryc_coords;
	child_poly.push_back(child_coords);
      break;
    }
    default: {
      LF_VERIFY_MSG(false,"refinement pattern " << (int)ref_pat_
		  << "not implemented for triangle!");
      break;
    }
    } // end switch ref_pat_
    break;
  } // end case of a triangle
  case lf::base::RefEl::kQuad(): {
    // Lattice coordinates of special points in a quadrilateral
    // Here we use knowledge about the conventions underlying node
    // and edge numbering in a quadrilateral as defined in RefEl.
    // This information could also be retrieved from the reference element.
    Eigen::MatrixXi lt_node_coords(2,4);
    lt_node_coords << 0,lt_one,lt_one,0,0,0,lt_one,lt_one;
    Eigen::MatrixXi lt_midpoint_coords(2,4);
    lt_midpoint_coords << lt_half,lt_one,lt_half,0,0,lt_half,lt_one,lt_half;

    // Remap local indices according to anchor values
    const int mod_0 = (0+anchor_)%4;
    const int mod_1 = (1+anchor_)%4;
    const int mod_2 = (2+anchor_)%4;
    const int mod_3 = (3+anchor_)%4;
    
    // For temporary storage of triangular lattice polygon
    Eigen::MatrixXi tria_child_coords(2,3);
    // For temporary storage of 4-node lattice polygon
    Eigen::MatrixXi quad_child_coords(2,4);
    switch (ref_pat_) {
    case RefPat::rp_nil: {
      break;
    }
     case (int)RefPat::rp_copy: {
       child_poly.push_back(lt_node_coords);
      break;
    }
    case (int)RefPat::rp_trisect: {
      LF_VERIFY_MSG(anchor_set_,
		    "Anchor must be set for trisection refinement of quad");

      // Partition a quad into three triangle, the anchor edge
      // being split in the process
	tria_child_coords.col(0) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(1) = lt_node_coords.col(mod_2);
	tria_child_coords.col(2) = lt_node_coords.col(mod_3);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(1) = lt_node_coords.col(mod_0);
	tria_child_coords.col(2) = lt_node_coords.col(mod_3);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(1) = lt_node_coords.col(mod_1);
	tria_child_coords.col(2) = lt_node_coords.col(mod_2);
	child_poly.push_back(tria_child_coords);
      break;
    }
    case (int)RefPat::rp_quadsect: {
      LF_VERIFY_MSG(anchor_set_,
		    "Anchor must be set for quadsection refinement of triangle");
      // Partition a quad into four triangle, thus 
      // splitting two edges. The one with the smaller sub index is the
      // anchor edge
	tria_child_coords.col(0) = lt_node_coords.col(mod_0);
	tria_child_coords.col(1) = lt_node_coords.col(mod_3);
	tria_child_coords.col(2) = lt_midpoint_coords.col(mod_0);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_node_coords.col(mod_1);
	tria_child_coords.col(1) = lt_midpoint_coords.col(mod_1);
	tria_child_coords.col(2) = lt_midpoint_coords.col(mod_0);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_node_coords.col(mod_2);
	tria_child_coords.col(1) = lt_node_coords.col(mod_3);
	tria_child_coords.col(2) = lt_midpoint_coords.col(mod_1);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(1) = lt_midpoint_coords.col(mod_1);
	tria_child_coords.col(2) = lt_node_coords.col(mod_3);
	child_poly.push_back(tria_child_coords);
      break;
    }
    case (int)RefPat::rp_bisect: 
    case (int)RefPat::rp_split: {
      LF_VERIFY_MSG(anchor_set_,
		    "Anchor must be set for splitting of quad");

      // Cut a quadrilateral into two 
	quad_child_coords.col(0) = lt_node_coords.col(mod_0);
	quad_child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	quad_child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	quad_child_coords.col(3) = lt_node_coords.col(mod_3);
	child_poly.push_back(quad_child_coords);

	quad_child_coords.col(0) = lt_node_coords.col(mod_1);
	quad_child_coords.col(1) = lt_node_coords.col(mod_2);
	quad_child_coords.col(2) = lt_midpoint_coords.col(mod_2);
	quad_child_coords.col(3) = lt_midpoint_coords.col(mod_0);
	child_poly.push_back(quad_child_coords);
      break;
    }
    case (int)RefPat::rp_threeedge: {
      LF_VERIFY_MSG(anchor_set_,
		    "Anchor must be set for three edge refinement of a quad");

	quad_child_coords.col(0) = lt_node_coords.col(mod_2);
	quad_child_coords.col(1) = lt_node_coords.col(mod_3);
	quad_child_coords.col(2) = lt_midpoint_coords.col(mod_3);
	quad_child_coords.col(3) = lt_midpoint_coords.col(mod_1);
	child_poly.push_back(quad_child_coords);

	tria_child_coords.col(0) = lt_node_coords.col(mod_0);
	tria_child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(2) = lt_midpoint_coords.col(mod_3);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_node_coords.col(mod_1);
	tria_child_coords.col(1) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(2) = lt_midpoint_coords.col(mod_1);
	child_poly.push_back(tria_child_coords);

	tria_child_coords.col(0) = lt_midpoint_coords.col(mod_0);
	tria_child_coords.col(1) = lt_midpoint_coords.col(mod_1);
	tria_child_coords.col(2) = lt_midpoint_coords.col(mod_3);
	child_poly.push_back(tria_child_coords);
      break;
    }
    case (int)RefPat::rp_barycentric:
    case (int)RefPat::rp_regular: {
      // Fully symmetric splitting into four quadrilaterals
      // Obtain coordinates of center of gravity
      Eigen::Vector2i lt_baryc_coords = Eigen::Vector2i({lt_half,lt_half});

      quad_child_coords.col(0) = lt_node_coords.col(0);
	quad_child_coords.col(1) = lt_midpoint_coords.col(0);
	quad_child_coords.col(2) = lt_baryc_coords;
	quad_child_coords.col(3) = lt_midpoint_coords.col(3);
	child_poly.push_back(quad_child_coords);

	quad_child_coords.col(0) = lt_node_coords.col(1);
	quad_child_coords.col(1) = lt_midpoint_coords.col(1);
	quad_child_coords.col(2) = lt_baryc_coords;
	quad_child_coords.col(3) = lt_midpoint_coords.col(0);
	child_poly.push_back(quad_child_coords);

	quad_child_coords.col(0) = lt_node_coords.col(2);
	quad_child_coords.col(1) = lt_midpoint_coords.col(1);
	quad_child_coords.col(2) = lt_baryc_coords;
	quad_child_coords.col(3) = lt_midpoint_coords.col(2);
	child_poly.push_back(quad_child_coords);

	quad_child_coords.col(0) = lt_node_coords.col(3);
	quad_child_coords.col(1) = lt_midpoint_coords.col(2);
	quad_child_coords.col(2) = lt_baryc_coords;
	quad_child_coords.col(3) = lt_midpoint_coords.col(3);
	child_poly.push_back(quad_child_coords);
      break;
    }
    default: {
      LF_VERIFY_MSG(false,"refinement pattern " << (int)ref_pat_
		  << "not implemented for quadrilateral!");
      break;
    }
    } // end switch ref_pat_
    break;
  } // end case of a quadrilateral
    
  } // end switch cell type 
  return(child_poly);
}

}
