/**
 * @file
 * @brief tests for geometry objects
 * @author Anian Ruoss
 * @date   2018-10-27 15:57:17
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include <lf/geometry/geometry.h>
#include <lf/quad/quad.h>
#include <lf/refinement/refinement.h>

/**
 * Checks if Jacobian() is implemented correctly by comparing it with the
 * symmetric difference quotient approximation
 */
void checkJacobians(const lf::geometry::Geometry &geom,
                    const Eigen::MatrixXd &eval_points,
                    const double tolerance) {
  const double h = 1e-6;

  const size_t num_points = eval_points.cols();
  const size_t dim_local = geom.DimLocal();
  const size_t dim_global = geom.DimGlobal();

  Eigen::MatrixXd jacobians = geom.Jacobian(eval_points);

  EXPECT_EQ(jacobians.rows(), dim_global) << "Jacobian has " << jacobians.rows()
                                          << " rows instead of " << dim_global;
  EXPECT_EQ(jacobians.cols(), num_points * dim_local)
      << "Jacobian has " << jacobians.cols() << " cols instead of "
      << num_points * dim_local;

  for (size_t j = 0; j < num_points; ++j) {
    auto point = eval_points.col(j);

    Eigen::MatrixXd jacobian =
        jacobians.block(0, j * dim_local, dim_global, dim_local);
    Eigen::MatrixXd approx_jacobian =
        Eigen::MatrixXd::Zero(dim_global, dim_local);

    for (size_t i = 0; i < dim_local; ++i) {
      Eigen::VectorXd h_vec = Eigen::VectorXd::Zero(dim_local);
      h_vec(i) = h;

      // approximate gradient with symmetric difference quotient
      approx_jacobian.col(i) =
          (geom.Global(point + h_vec) - geom.Global(point - h_vec)) / (2. * h);
    }

    EXPECT_TRUE(jacobian.isApprox(approx_jacobian, tolerance))
        << "Jacobian incorrect at point " << point;
  }
}

/**
 * Checks if JacobianInverseGramian() is implemented correctly under the
 * assumption that Jacobian() is correct
 */
void checkJacobianInverseGramian(const lf::geometry::Geometry &geom,
                                 const Eigen::MatrixXd &eval_points) {
  const size_t num_points = eval_points.cols();
  const size_t dim_local = geom.DimLocal();
  const size_t dim_global = geom.DimGlobal();

  Eigen::MatrixXd jacobians = geom.Jacobian(eval_points);
  Eigen::MatrixXd jacInvGrams = geom.JacobianInverseGramian(eval_points);

  EXPECT_EQ(jacInvGrams.rows(), dim_global)
      << "JacobianInverseGramian has " << jacInvGrams.rows()
      << " rows instead of " << dim_global;
  EXPECT_EQ(jacInvGrams.cols(), num_points * dim_local)
      << "JacobianInverseGramian has " << jacInvGrams.cols()
      << " cols instead of " << num_points * dim_local;

  for (int j = 0; j < num_points; ++j) {
    Eigen::MatrixXd jacInvGram =
        jacInvGrams.block(0, j * dim_local, dim_global, dim_local);
    Eigen::MatrixXd jacobian =
        jacobians.block(0, j * dim_local, dim_global, dim_local);

    EXPECT_TRUE(jacInvGram.isApprox(
        jacobian * (jacobian.transpose() * jacobian).inverse()))
        << "JacobianInverseGramian incorrect at point " << eval_points.col(j);
  }
}

/**
 * Checks if IntegrationElement() is implemented correctly under the assumption
 * that Jacobian() is correct
 */
void checkIntegrationElement(const lf::geometry::Geometry &geom,
                             const Eigen::MatrixXd &eval_points) {
  const size_t num_points = eval_points.cols();
  const size_t dim_local = geom.DimLocal();
  const size_t dim_global = geom.DimGlobal();

  Eigen::MatrixXd jacobians = geom.Jacobian(eval_points);
  Eigen::VectorXd integrationElements = geom.IntegrationElement(eval_points);

  EXPECT_EQ(integrationElements.rows(), num_points)
      << "IntegrationElement has " << integrationElements.rows()
      << " rows instead of " << num_points;
  EXPECT_EQ(integrationElements.cols(), 1)
      << "IntegrationElement has " << integrationElements.cols()
      << " cols instead of " << 1;

  for (int j = 0; j < num_points; ++j) {
    Eigen::MatrixXd jacobian =
        jacobians.block(0, j * dim_local, dim_global, dim_local);

    const double integrationElement = integrationElements(j);
    const double approx_integrationElement =
        std::sqrt((jacobian.transpose() * jacobian).determinant());

    EXPECT_DOUBLE_EQ(integrationElement, approx_integrationElement)
        << "IntegrationElement incorrect at point " << eval_points.col(j);
  }
}

/**
 * Checks that SubGeometry and Geometry map the same nodes to the same points
 */
void checkSubGeometry(const lf::geometry::Geometry &geom) {
  // nodeCoords is a (refEl.Dimension, refEl.NumNodes) matrix
  const auto refEl = geom.RefEl();
  const Eigen::MatrixXd &nodeCoords = refEl.NodeCoords();

  // iterate over all relative codimensions
  for (size_t codim = 0; codim <= geom.RefEl().Dimension(); ++codim) {
    // iterate over all subEntities in given codimension
    const auto numSubEntities = refEl.NumSubEntities(codim);
    for (size_t subEntity = 0; subEntity < numSubEntities; ++subEntity) {
      // subNodeCoords is a (subRefEl.Dimension, subRefEl.NumNodes) matrix
      auto subGeom = geom.SubGeometry(codim, subEntity);
      auto subRefEl = subGeom->RefEl();
      const Eigen::MatrixXd &subNodeCoords = subRefEl.NodeCoords();

      // iterate over all nodes of subEntity
      for (size_t subNode = 0; subNode < subRefEl.NumNodes(); ++subNode) {
        // map coordinates in subRefEl.Dimension to geom.DimGlobal
        auto globalCoordsFromSub = subGeom->Global(subNodeCoords.col(subNode));
        // get index of subSubEntity with respect to refEl
        const int subSubIdx = refEl.SubSubEntity2SubEntity(
            codim, subEntity, geom.DimLocal() - codim, subNode);
        // map coordinates in RefEl.Dimension to geom.DimGlobal
        auto globalCoords = geom.Global(nodeCoords.col(subSubIdx));

        EXPECT_EQ(globalCoordsFromSub, globalCoords)
            << "Global mapping of subNode " << subNode << " of subEntity "
            << subEntity << " in relative codim " << codim
            << " differs from global mapping of node " << subSubIdx;
      }
    }
  }
}

void runGeometryChecks(const lf::geometry::Geometry &geom,
                       const Eigen::MatrixXd &eval_points,
                       const double tolerance) {
  checkJacobians(geom, eval_points, tolerance);
  checkJacobianInverseGramian(geom, eval_points);
  checkIntegrationElement(geom, eval_points);
  checkSubGeometry(geom);
}

/**
 * Check if the total volume is conserved after call to ChildGeometry()
 */
void checkChildGeometryVolume(
    const lf::geometry::Geometry &geom, const lf::refinement::RefPat &refPat,
    const lf::base::sub_idx_t &anchor = lf::refinement::idx_nil) {
  const double volume = lf::geometry::Volume(geom);
  double refinedVolume = 0.;
  auto children = geom.ChildGeometry(
      lf::refinement::Hybrid2DRefinementPattern(geom.RefEl(), refPat, anchor),
      0);

  for (auto &childGeom : children) {
    refinedVolume += lf::geometry::Volume(*childGeom);
  }

  switch (refPat) {
    case lf::refinement::RefPat::rp_nil:
      EXPECT_EQ(refinedVolume, 0.)
          << refPat << " should not produce any children";
      break;
    default:
      EXPECT_EQ(volume, refinedVolume)
          << "Parent and children volumes differ for " << refPat;
  }
}

TEST(Geometry, Point) {
  lf::geometry::Point geom((Eigen::MatrixXd(2, 1) << 1, 1).finished());
  // QuadRule is not implemented and coordinate values are irrelevant
  Eigen::MatrixXd points = Eigen::MatrixXd::Random(0, 3);
  runGeometryChecks(geom, points, 1e-9);

  std::vector<lf::refinement::RefPat> pointSymmetricRefPats = {
      lf::refinement::RefPat::rp_copy};

  for (const auto &refPat : pointSymmetricRefPats) {
    checkChildGeometryVolume(geom, refPat);
  }
}

TEST(Geometry, SegmentO1) {
  lf::geometry::SegmentO1 geom(
      (Eigen::MatrixXd(2, 2) << 1, 1, 0, 4).finished());
  auto qr = lf::quad::make_QuadRule(lf::base::RefEl::kSegment(), 5);
  runGeometryChecks(geom, qr.Points(), 1e-9);

  std::vector<lf::refinement::RefPat> segSymmetricRefPats = {
      lf::refinement::RefPat::rp_nil, lf::refinement::RefPat::rp_copy,
      lf::refinement::RefPat::rp_split};

  for (const auto &refPat : segSymmetricRefPats) {
    checkChildGeometryVolume(geom, refPat);
  }
}

TEST(Geometry, TriaO1) {
  lf::geometry::TriaO1 geom(
      (Eigen::MatrixXd(2, 3) << 1, 4, 3, 1, 2, 5).finished());
  auto qr = lf::quad::make_QuadRule(lf::base::RefEl::kTria(), 5);
  runGeometryChecks(geom, qr.Points(), 1e-9);

  std::vector<lf::refinement::RefPat> triaSymmetricRefPats = {
      lf::refinement::RefPat::rp_nil, lf::refinement::RefPat::rp_copy,
      lf::refinement::RefPat::rp_regular,
      lf::refinement::RefPat::rp_barycentric};

  for (const auto &refPat : triaSymmetricRefPats) {
    checkChildGeometryVolume(geom, refPat);
  }

  std::vector<lf::refinement::RefPat> triaAsymmetricRefPats = {
      lf::refinement::RefPat::rp_bisect, lf::refinement::RefPat::rp_trisect,
      lf::refinement::RefPat::rp_trisect_left,
      lf::refinement::RefPat::rp_quadsect};

  for (const auto &refPat : triaAsymmetricRefPats) {
    for (size_t anchor = 0; anchor < 3; ++anchor) {
      checkChildGeometryVolume(geom, refPat, anchor);
    }
  }
}

TEST(Geometry, QuadO1) {
  lf::geometry::QuadO1 geom(
      (Eigen::MatrixXd(2, 4) << -1, 3, 2, 1, -2, 0, 2, 1).finished());
  auto qr = lf::quad::make_QuadRule(lf::base::RefEl::kQuad(), 5);
  runGeometryChecks(geom, qr.Points(), 1e-9);

  std::vector<lf::refinement::RefPat> quadSymmetricRefPats = {
      lf::refinement::RefPat::rp_nil, lf::refinement::RefPat::rp_copy,
      lf::refinement::RefPat::rp_regular,
      lf::refinement::RefPat::rp_barycentric};

  for (const auto &refPat : quadSymmetricRefPats) {
    checkChildGeometryVolume(geom, refPat);
  }

  std::vector<lf::refinement::RefPat> triaAsymmetricRefPats = {
      lf::refinement::RefPat::rp_split, lf::refinement::RefPat::rp_bisect,
      lf::refinement::RefPat::rp_trisect, lf::refinement::RefPat::rp_quadsect,
      lf::refinement::RefPat::rp_threeedge};

  for (const auto &refPat : triaAsymmetricRefPats) {
    for (size_t anchor = 0; anchor < 4; ++anchor) {
      checkChildGeometryVolume(geom, refPat, anchor);
    }
  }
}
