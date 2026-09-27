# Contributing

Thank you for helping improve the SPECS Optical Compensation Layer.

## Development workflow

1. Create a focused branch from the default branch.
2. Add tests for behavioral changes.
3. Run the complete local test suite:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ctest --test-dir build --output-on-failure
   python -m unittest discover -s tests -p 'test_*.py'
   ```

4. Open a pull request that explains the motivation, implementation, and any
   integration impact.

Keep the core dependency-light and avoid coupling it to a particular graphics
loader, window system, or device SDK. By participating, you agree to follow the
project's [Code of Conduct](CODE_OF_CONDUCT.md).
