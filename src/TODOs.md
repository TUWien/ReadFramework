# Refactoring
- QLine Line::line() const; -> QLine Line::qLine() const;
- Clean up `ImageProcessor` and `Algorithm`
  - e.g. Algoritm::get1DGauss
- Sort poly coordinates to reflect tl orientation
