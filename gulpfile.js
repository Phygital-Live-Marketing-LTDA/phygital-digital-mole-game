const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const cleanCSS = require('gulp-clean-css');
const terser = require('gulp-terser');
const rename = require('gulp-rename');

// Minifica arquivos HTML
function minifyHtml() {
  return gulp.src('public/**/*.html')
    .pipe(htmlmin({ collapseWhitespace: true }))
    .pipe(gulp.dest('data'));
}

// Minifica arquivos CSS
function minifyCss() {
  return gulp.src('public/**/*.css')
    .pipe(cleanCSS({ compatibility: 'ie8' }))
    .pipe(gulp.dest('data'));
}

// Minifica arquivos JavaScript
function minifyJs() {
  return gulp.src('public/**/*.js')
    .pipe(terser())
    .pipe(gulp.dest('data'));
}

// Copia demais arquivos (imagens, fonts, etc.)
function copyOther() {
  return gulp.src(['public/**/*', '!public/**/*.html', '!public/**/*.css', '!public/**/*.js'])
    .pipe(gulp.dest('data'));
}

// Tarefa principal de build
const build = gulp.parallel(minifyHtml, minifyCss, minifyJs);

exports.build = build;
