#ifndef HW_IMPURITY_TRANSPORT_HWITSYSTEM_H
#define HW_IMPURITY_TRANSPORT_HWITSYSTEM_H

#include "../Diagnostics/GrowthRatesRecorder.hpp"
#include "../ParticleSystems/ParticleSystem.hpp"

#include "nektar_interface/solver_base/time_evolved_eqnsys_base.hpp"
#include "nektar_interface/utilities.hpp"

#include <LibUtilities/LinearAlgebra/NekNonlinSysIter.h>
#include <LibUtilities/Memory/NekMemoryManager.hpp>
#include <SolverUtils/AdvectionSystem.h>
#include <SolverUtils/Core/Misc.h>
#include <SolverUtils/EquationSystem.h>
#include <SolverUtils/Forcing/Forcing.h>
#include <SolverUtils/RiemannSolvers/RiemannSolver.h>

#include <solvers/solver_callback_handler.hpp>

namespace LU = Nektar::LibUtilities;
namespace MR = Nektar::MultiRegions;
namespace SD = Nektar::SpatialDomains;
namespace SU = Nektar::SolverUtils;

namespace NESO::Solvers::hw_impurity_transport {

/**
 * @brief Equation system for the HW impurity transport proxyapp
 *
 */
class HWITSystem
    : public TimeEvoEqnSysBase<SU::UnsteadySystem, ParticleSystem> {
public:
  friend class MemoryManager<HWITSystem>;

  /// Name of class
  static std::string class_name;

  /**
   * @brief Create an instance of this class and initialise it.
   */
  static SU::EquationSystemSharedPtr
  create(const LU::SessionReaderSharedPtr &session,
         const SD::MeshGraphSharedPtr &graph) {
    SU::EquationSystemSharedPtr p =
        MemoryManager<HWITSystem>::AllocateSharedPtr(session, graph);
    p->InitObject();
    return p;
  }

  /// Object to facilitate allows optional recording of energy and enstrophy
  std::shared_ptr<GrowthRatesRecorder<MR::DisContField>>
      energy_enstrophy_recorder;
  /// Callback handler to call user-defined callbacks
  SolverCallbackHandler<HWITSystem> solver_callback_handler;

protected:
  HWITSystem(const LU::SessionReaderSharedPtr &session,
             const SD::MeshGraphSharedPtr &graph);

  /// Advection object used in the electron density equation
  SU::AdvectionSharedPtr adv_obj;
  /// Advection type
  std::string adv_type;
  /// Hasegawa-Wakatani α
  NekDouble alpha;
  /// Magnetic field vector
  std::vector<NekDouble> B;
  /// Implicit solver parameter
  NekDouble bnd_evaluate_time = 0.0;
  /// Normalised magnetic field vector
  std::vector<NekDouble> b_unit;
  /** Source fields cast to DisContFieldSharedPtr, indexed by name, for use in
   * particle evaluation/projection methods
   */
  std::map<std::string, MR::DisContFieldSharedPtr> discont_fields;
  /// Bool to enable/disable growth rate recordings
  bool energy_enstrophy_recording_enabled;
  /// Storage for ExB drift velocity
  Array<OneD, Array<OneD, NekDouble>> ExB_vel;
  /// Implicit solver counter
  int imp_stages_counter = 0;
  /// Implicit solver parameter
  NekDouble in_arr_norm = -1.0;
  /// Implicit solver parameter
  NekDouble jacobi_free_eps = 5.0E-08;
  /// Hasegawa-Wakatani κ
  NekDouble kappa;
  /// Magnitude of the magnetic field
  NekDouble mag_B;
  /// Implicit solver counter
  int newton_its_counter = 0;
  /// Implicit solver counter
  int lin_its_counter = 0;
  /// Nektar non-linear system
  LibUtilities::NekNonlinSysIterSharedPtr nonlin_sys;
  /// Riemann solver type (used for all advection terms)
  std::string riemann_solver_type;
  /// Implicit solver parameter
  NekDouble time_int_lambda = 0.0;

  void calc_init_phi_and_gradphi();
  void calc_ref_vals(const Array<OneD, const NekDouble> &in_arr);
  void compute_grad_phi();
  void do_null_precon(const Array<OneD, const NekDouble> &in_arr,
                      Array<OneD, NekDouble> &out_arr, const bool &flag);

  void
  explicit_time_int(const Array<OneD, const Array<OneD, NekDouble>> &in_arr,
                    Array<OneD, Array<OneD, NekDouble>> &out_arr,
                    const NekDouble time);

  Array<OneD, NekDouble> &
  get_adv_vel_norm(Array<OneD, NekDouble> &trace_vel_norm,
                   const Array<OneD, Array<OneD, NekDouble>> &adv_vel);

  void get_flux_vector(const Array<OneD, Array<OneD, NekDouble>> &fields_vals,
                       const Array<OneD, Array<OneD, NekDouble>> &adv_vel,
                       Array<OneD, Array<OneD, Array<OneD, NekDouble>>> &flux);

  void
  get_phi_solve_rhs(const Array<OneD, const Array<OneD, NekDouble>> &in_arr,
                    Array<OneD, NekDouble> &rhs);
  void
  implicit_time_int(const Array<OneD, const Array<OneD, NekDouble>> &in_arr,
                    Array<OneD, Array<OneD, NekDouble>> &out_arr,
                    const NekDouble time, const NekDouble lambda);
  void implicit_time_int_1D(const Array<OneD, const NekDouble> &in_arr1D,
                            Array<OneD, NekDouble> &out_arr1D);
  void init_nonlin_sys_solver(void);
  virtual void load_params() override;
  void matrix_multiply_matrix_free(const Array<OneD, const NekDouble> &in_arr,
                                   Array<OneD, NekDouble> &out_arr,
                                   [[maybe_unused]] const bool &flag);
  void
  nonlin_sys_evaluator(const Array<OneD, const Array<OneD, NekDouble>> &in_arr,
                       Array<OneD, Array<OneD, NekDouble>> &out_arr);
  void nonlin_sys_evaluator_1D(const Array<OneD, const NekDouble> &in_arr,
                               Array<OneD, NekDouble> &out_arr,
                               [[maybe_unused]] const bool &flag);
  void solve_phi(const Array<OneD, const Array<OneD, NekDouble>> &in_arr);

  virtual void v_GenerateSummary(SU::SummaryList &s) override;
  virtual void v_InitObject(bool DeclareField) override;
  virtual bool v_PostIntegrate(int step) override;
  virtual bool v_PreIntegrate(int step) override;
  virtual void v_SetInitialConditions(NekDouble init_time, bool dump_ICs,
                                      const int domain) override;

private:
  /// d00 coefficient for Helmsolve
  NekDouble d00;
  /// d11 coefficient for Helmsolve
  NekDouble d11;
  /// d22 coefficient for Helmsolve
  NekDouble d22;
  /// Storage for component of ne advection velocity normal to trace elements
  Array<OneD, NekDouble> norm_vel_elec;
  /// Number of particle timesteps per fluid timestep.
  int num_part_substeps;
  /// Number of time steps between particle trajectory step writes.
  int particle_output_freq;
  /// Particle timestep size.
  double part_timestep;
  /// Riemann solver object used in electron advection
  SU::RiemannSolverSharedPtr riemann_solver;

  void
  do_ode_projection(const Array<OneD, const Array<OneD, NekDouble>> &in_arr,
                    Array<OneD, Array<OneD, NekDouble>> &out_arr,
                    const NekDouble time);
  Array<OneD, NekDouble> &get_adv_vel_norm_elec();

  void get_flux_vector_diff(
      const Array<OneD, Array<OneD, NekDouble>> &in_arr,
      const Array<OneD, Array<OneD, Array<OneD, NekDouble>>> &q_field,
      Array<OneD, Array<OneD, Array<OneD, NekDouble>>> &viscous_tensor);
  void
  get_flux_vector_elec(const Array<OneD, Array<OneD, NekDouble>> &fields_vals,
                       Array<OneD, Array<OneD, Array<OneD, NekDouble>>> &flux);
};

} // namespace NESO::Solvers::hw_impurity_transport
#endif // HW_IMPURITY_TRANSPORT_DRIFT_REDUCED_SYSTEM_H
