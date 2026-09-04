# Resources 目录说明

本目录用于存放应用的资源文件，包括但不限于：

## 目录结构
```
resources/
├── app.qrc          # Qt 资源文件（注册资源到应用）
├── icons/           # 图标目录
│   └── app.ico       # 应用程序图标
├── styles/          # QSS 样式表（可选）
├── translations/    # 国际化翻译文件（可选）
└── README.txt       # 本说明文件
```

## 资源文件类型

### 1. 图标 (icons/)
- `app.ico` - Windows 应用程序图标（建议 256x256 像素，包含多种尺寸）
- 其他图标：工具栏图标、对话框图标等（PNG 格式，建议 24x24 或 32x32）

### 2. 样式表 (styles/)
- Qt Style Sheet (.qss) 文件，用于自定义界面外观
- 可按功能模块分文件，如 `dark.qss`、`light.qss`

### 3. 翻译 (translations/)
- Qt Linguist 生成的翻译文件 (.qm)
- 源文件 (.ts) 由 lupdate 工具生成

## 添加新资源
1. 将文件放入对应子目录
2. 在 `app.qrc` 中添加对应的 `<file>` 条目
3. 运行 CMake 构建时会自动编译资源

## 注意事项
- 图标文件建议使用 256x256 像素的 ICO 格式（Windows）或 PNG 格式
- 资源路径以 `/` 为前缀，可通过 `:/` 访问，如 `:/icons/app.ico`
- 大型资源文件（如视频、数据库模板）建议放在资源目录之外，通过文件系统访问