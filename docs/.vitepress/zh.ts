import { defineConfig, type DefaultTheme } from 'vitepress'

const lastUpdate = new Date().toISOString().slice(0, 10)

export const zh = defineConfig({
    lang: 'zh-CN',

    description: 'C++异步运行时框架',

    themeConfig: {
        nav: makeNavBar(),

        sidebar: {
            '/zh/zedio/': { base: '/zh/zedio/', items: makeSidebarZEDIO() },
            '/zh/user_guide/': { base: '/zh/user_guide/', items: makeSidebarUserGuide() },
        },

        editLink: {
            pattern: 'https://github.com/8sileus/zedio/edit/main/docs/:path',
            text: '编辑此页面'
        },

        footer: {
            message: 'Released under the <a href="https://github.com/8sileus/zedio/blob/main/LICENSE">MIT License</a>',
            copyright: 'Copyright © 2024 <a href="https://github.com/8sileus">8sileus</a>'
        },
    }
})

function makeNavBar(): DefaultTheme.NavItem[] {
    return [
        {
            text: `Zedio(${lastUpdate})`,
            items: [
                {
                    text: '关于',
                    link: '/zh/zedio/overview'
                },
                {
                    text: '安装 & 使用',
                    link: '/zh/zedio/install'
                },
                {
                    text: '参与贡献',
                    link: '/zh/zedio/contributing'
                },
            ],
            activeMatch: '/zh/zedio/'
        },
        //! can be fold into Zedio
        {
            text: '基准测试',
            link: '/zh/zedio/benchmark'
        },
        {
            text: 'Runtime',
            link: '/zh/user_guide/runtime'
        },
        {
            text: 'Async IO',
            link: '/zh/user_guide/io'
        },
    ]
}

function makeSidebarZEDIO(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Zedio',
            collapsed: false,
            items: [
                { text: '关于', link: 'overview' },
                { text: '安装 & 使用', link: 'install' },
                { text: '基准测试', link: 'benchmark' },
                { text: '参与贡献', link: 'contributing' },
            ]
        },
    ]
}

function makeSidebarUserGuide(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'API',
            collapsed: false,
            items: [
                { text: 'Runtime', link: 'runtime' },
                { text: 'Async IO', link: 'io' },
            ]
        },
    ]
}
