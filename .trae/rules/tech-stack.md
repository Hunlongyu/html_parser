现代化C++开发项目规范：

【C++语言标准】
- 统一使用 C++17/C++20 标准
- 禁用未稳定的新特性
- 优先包含头文件 <ranges> 替代 <algorithm>
- 优先使用现代C++特性：智能指针、auto、span、concepts、coroutine等

【代码风格设计】
- 遵循单一职责原则
- RAII资源管理模式
- 参数优先const引用传递
- 成员函数可const时必须声明为const
- 优先使用composition而非inheritance
- 采用一致的命名风格(推荐camelCase)
- 避免深层嵌套，每函数不超过50行

【注释与文档】
- 接口使用Doxygen格式注释
- 复杂逻辑添加设计意图说明
- 关键变量提供含义和使用周期注释
- 使用TODO/FIXME标记待办事项
- 模块入口提供使用示例

【构建系统】
- CMake 3.20+ 版本管理
- 支持out-of-tree构建
- 提供Debug/Release配置
- 输出静态库(.lib/.a)和动态库(.dll/.so/.dylib)两种类型
- 预设全面的编译警告，零警告提交

【项目结构】
- src主源码目录
- include头文件目录
- test单元测试目录
- example示例代码目录
- doc文档目录
- cmake自定义模块目录

【质量保证】
- 单元测试驱动开发
- CI流水线集成代码质量检测
- 覆盖率要求>85%
- 内存泄漏自动检测
- 静态代码分析工具集成

【工程实践】
- Git版本管理，约定式提交(commitizen)
- 语义化版本规范SemVer
- issue-driven开发流程
- 提供详细的README.md项目说明
- 支持跨平台构建(Windows/Linux/macOS)

【性能安全】
- 避免裸指针使用
- 启用ASLR/NX保护机制
- 边界检查防御缓冲区溢出
- 异常安全等级保证
- 移动语义优化隐式拷贝