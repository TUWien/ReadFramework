# Refactoring
- Clean up `ImageProcessor` and `Algorithm`
  - e.g. Algoritm::get1DGauss
- Sort poly coordinates to reflect tl orientation
- Check includes (e.g. #include `"opencv/.."` vs `#include <opencv/..>`)
- rename `LabelInfo::label()` -> `LabelInfo::predicted()`
