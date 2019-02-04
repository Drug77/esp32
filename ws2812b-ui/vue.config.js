// vue.config.js
module.exports = {
    css: {
        loaderOptions: {
            sass: {
                data: `
                  @import "@/scss/app.scss";
                `
            }
        }
    },
    configureWebpack: {
        // No need for splitting
        optimization: {
            splitChunks: false
        },
        output: {
            filename: '[name].js',
            chunkFilename: '[name].js'
        },
        /*module: {
            rules: [
                {
                    test: /\.(woff2?|eot|ttf|otf)(\?.*)?$/i,
                    loader: 'file-loader',
                    options: {
                        name: '[name].[ext]',
                        outputPath: 'a',
                        publicPath: 's',
                    },
                },
            ]
        },*/
    },
    chainWebpack: config => {
        if (config.plugins.has('extract-css')) {
            const extractCSSPlugin = config.plugin('extract-css')
            extractCSSPlugin && extractCSSPlugin.tap(() => [{
                filename: '[name].css',
                chunkFilename: '[name].css'
            }])
        }
    },
}