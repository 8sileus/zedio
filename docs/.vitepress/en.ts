import { defineConfig, type DefaultTheme } from 'vitepress'

const lastUpdate = new Date().toISOString().slice(0, 10)

export const en = defineConfig({
    lang: 'en-US',

    description: 'Asynchronous Library for Applications in C++',

    themeConfig: {
        nav: makeNavBar(),

        sidebar: {
            '/zedio/': { base: '/zedio/', items: makeSidebarZEDIO() },
            '/user_guide/': { base: '/user_guide/', items: makeSidebarUserGuide() },
        },

        editLink: {
            pattern: 'https://github.com/8sileus/zedio/edit/main/docs/:path',
            text: 'Edit this page on Github'
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
                    text: 'About',
                    link: '/zedio/overview'
                },
                {
                    text: 'Install & Usage',
                    link: '/zedio/install'
                },
                {
                    text: 'Contributing',
                    link: '/zedio/contributing'
                },
            ],
            activeMatch: '/zedio/'
        },
        //! can be fold into Zedio
        {
            text: 'Benchmark',
            link: '/zedio/benchmark'
        },
        {
            text: 'Runtime',
            link: '/user_guide/runtime'
        },
        {
            text: 'Async IO',
            link: '/user_guide/io'
        },
    ]
}

function makeSidebarZEDIO(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Zedio',
            collapsed: false,
            items: [
                { text: 'Overview', link: 'overview' },
                { text: 'Install & Usage', link: 'install' },
                { text: 'Benchmark', link: 'benchmark' },
                { text: 'Contributing', link: 'contributing' },
            ]
        },
    ]
}

function makeSidebarUserGuide(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'User Guide',
            collapsed: false,
            items: [
                { text: 'Runtime', link: 'runtime' },
                { text: 'Async IO', link: 'io' },
            ]
        },
    ]
}
