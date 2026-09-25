# Injector laboratory

The injector work is separated from the turbopump because injector pressure
drop, atomization and mixture distribution are downstream acceptance problems.

## Evidence from the notes

- `LOX_rp.txt`: pintle ranges such as TMR, spray angle, BF and Dc/Dp.
- `tests_conditions.txt`: gas/liquid atomization tests with pentad and coaxial
  elements, useful for method comparison but based on surrogate fluids.
- `rocketdesign.txt`: orifice relation `m_dot = Cd A sqrt(2 rho DeltaP)`.
- `other-books/cohete.txt`: historical injector and manifold arrangements.

The first implementation should calculate required orifice area and preserve
the test-fluid identity. It must not infer LOX/CH4 spray performance from
water, wax, glycerol or salt-water tests.