Base is a framework built specifically for HHVM/Hack. It does not run on PHP. We wanted to make Base simple to understand, configure and use, and possibly performant.

Base is the ideal RAD framework, but it is also powering production apps.

----------

Highlights
---------------

- **Small and simple (for real).** This means few files, almost zero dependencies, little configuration, but everything to get you going. With a small set of APIs that just make sense, you can get started right after you clone this repo.
- **Atomic controllers.** A route maps to one controller, and one controller can handle one action only.
- **MWC (Model-Widget-Controller).** Templating has never been so easy. Write XHP layouts and widgets for a super-fluent syntax and full XSS protection.
- **Not-so-opinionated.** Don't like our templating? Sure, use Handlebars. Don't like our ORM? Plug Doctrine in. We love difference.

Features
-------------
 - **Built-in XHP localisation** via `:t` and `:t` tags
 - **Layouts and widgets**: `:base:layout` and `:base:widget`, built on top of XHP, too.
 - **Built-in workers and message queue**: supports Redis for async communication with external web services and APIs
 - **Event bus**: `ApiRunner::addEventListener` and `ApiRunner::fireEvent` allows you to extend Base by creating custom hooks
 - **Built-in MongoDB ODM.** Strongly type your documents using `BaseModel`. Link documents and automatically resolve references using `BaseRef`.
 - **Route parameters.** Specify a route map and link parameters. Reference routes by their names, not their URLs. Create route aliases (one controller, multiple routes).
 - **Request parameter validation and casting.** Specify which parameter your controller must accept, and Base performs validation, sanitisation and type casting accordingly. Everything else is discarded.

Contribute
---------------
Ever wanted to drive a new framework to widespread popularity? Base is the right chance. There are specific areas in which you can make an impact:
- **Test.** We would like to build our custom test framework, and increase our test coverage.
- **Discussions.** Let's crystallise Base's design principles and grow from there.
- **Full Hack syntax.** We want Base to run on `strict` only on the long run. Currently, we are using gradual typing and arrays, but we want to be more specific on that.
- **Documentation.** Although you should be able to understand what Base is doing by simply looking at its code, it would be great to have tutorials and documentation to help people build great apps.

License
----------
MIT
