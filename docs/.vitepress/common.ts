import { defineConfig, type DefaultTheme } from 'vitepress'

export const common = defineConfig({
    base: '/zedio/',

    title: 'ZEDIO',

    lastUpdated: true,
    cleanUrls: true,
    metaChunk: true,

    themeConfig: {
        socialLinks: [
            { icon: 'github', link: 'https://github.com/8sileus/zedio' }
        ],

        search: {
            provider: 'local',
        },
    }
})
