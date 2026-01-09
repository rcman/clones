# BCC-500 vs SDS 940 Architecture Comparison

## Answer: No, they are significantly different

The BCC-500 and SDS 940 architectures have important differences that make direct implementation of SDS 940 approaches problematic.

## Key Architectural Differences

### SDS 940
- Single CPU architecture (modified SDS 930)
- 24-bit words
- 18-bit addressing (can address 256K words)
- Traditional single-processor design with memory management unit
- Hardware memory paging with 2K-word pages
- User/Monitor modes for protection

### BCC-500
- Six independent microcoded processors (multiprocessor design)
- 24-bit words (same)
- 18-bit addressing (same bit width)
- Two processors for user code with expanded capabilities
- Four specialized processors each handling different OS functions
- Hybrid microcode/macrocode implementation
- Implemented "a subset of the 940 instruction set" (not complete compatibility)

## Critical Implications

The BCC-500's multiprocessor architecture with specialized OS processors is fundamentally different from the SDS 940's single-processor design. While the BCC-500 intentionally implemented a subset of 940 instructions (likely for software compatibility), the underlying execution model is radically different.

## Recommendation

You'll likely need to create an adapted startup sequence rather than directly implementing the SDS 940 ICL approach because:

1. The multiprocessor coordination required for BCC-500 boot
2. Different processor roles (user vs. OS processors)
3. The microcode layer that sits beneath the instruction set
4. Different memory management implementations

## Next Steps

Would you be able to share what specific SDS 940 ICL startup sequence documentation you're working from? That would help provide more specific guidance on the adaptations needed.
