import { defineConfig } from 'vitepress'
import { common } from './common'
import { en } from './en'
import { zh } from './zh'

export default defineConfig({
    ...common,

    locales: {
        root: { label: 'English', ...en },
        zh: { label: '简体中文(未完成)', ...zh }
    }
})