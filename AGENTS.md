# Agent Operating Contract

## Objective

Move quickly with minimal human interaction while keeping this security-sensitive project safe, traceable, and affordable.

## Source-of-truth order

1. `security.md`
2. `functional.md`
3. `architecture.md`
4. `tasks.md`
5. Existing code and tests

If two sources conflict, stop and report the conflict instead of inventing a resolution.

## Roles

### Planner

- May edit: `spec/tasks.md`, proposals in `reports/`.
- May not edit: production code, cryptographic code, build/release configuration.
- Output: small tasks with dependencies, acceptance criteria, and assigned owner.

### Architect

- May edit: `spec/architecture.md`, repository structure, build topology, interface definitions.
- May not weaken: `spec/security.md`.
- Output: explicit module/file ownership and migration notes.

### Coder

- May edit: only files named in its assigned task plus directly necessary tests.
- Must: run the available build/lint/test commands, update task status, and give a compact change report.
- Must not: add unreviewed crypto, change security invariants, access unrelated user files, or use external secrets.

### Tester

- May edit: tests, test fixtures, test documentation, and small correctness fixes approved by task scope.
- Must: reproduce failures before declaring a fix.
- Output: commands run, result, failed tests, and smallest useful reproduction.

### Security reviewer

- May edit: `reports/security-findings.md` and review comments.
- Must not directly silently alter security-critical code.
- Output: severity, affected files, exploit condition, required remediation, and verification step.

### Packager

- May edit: package manifests, build/release scripts, and release documentation.
- Must not: publish, sign with production keys, or upload artifacts without explicit human approval.

## Cost and token policy

- Supply agents only the assigned task, applicable spec sections, relevant source files, and relevant test output.
- Do not paste the whole repository into a model prompt.
- Use cheaper/local models for file inventory, formatting, documentation, routine tests, and simple compile fixes.
- Use stronger models for architecture decisions, protocol work, cryptography review, and final security review.
- Cache summaries in files; do not pay to rediscover repo facts.
- Cap autonomous repair cycles per task at three; then create a concise blocker report.

## Command policy

Pre-approved commands may include project-local build, test, format, static analysis, package, and version-control status/diff operations.

Disallowed without explicit human approval:

- Registry modification
- System-wide installation or removal
- Production publishing
- Access to unrelated directories
- Reading, emitting, or changing credentials and secret stores
- Network access beyond explicitly approved source/documentation dependencies

## Merge gate

A task can be marked `DONE` only when:

- Its acceptance criteria are satisfied.
- Relevant tests/build checks pass or a documented platform limitation is attached.
- The diff stays within its authorized scope.
- Security review approves sensitive changes.
- The human owner approves the final pull request for milestone merges.
