var equal = require('assert').equal

var verity = require('verity').createServer('api-key')
var browser = verity.connect('chrome', _)

browser.open('https://www.prettyrobots.com/form', _)

ok(browser.run(function () {
    this.helpers = {
        normalize: 'foo',
        create: 'bar'
    }
})

ok(browser.run(function ($, syn) {
    $('input.name').val('Alan Gutierrez')
    $('input.email').val('alan@prettyrobots.com')
    $('input.submit').click()
    return true
}, _), 'submitted')

browser.until(function () {
    return /thank-you.html$/.test(window.location)
}, _)

ok(browser.run(function ($) {
    return /Alan Gutierrez/.test($('div.message').val())
}, _), 'thanked')
